#include "Updater.h"
#include "Config.h"
#include "Language.h"
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <ctime>
#include <cwchar>
#include <cstdlib>
#include <cstring>

#ifndef BCRYPT_ECDSA_PUBLIC_P256_MAGIC
#define BCRYPT_ECDSA_PUBLIC_P256_MAGIC 0x31534345
#endif

namespace updater {

    namespace {

        const wchar_t* kRegKey = L"Software\\ZimoRecorder\\Update";

        // ---------------------------------------------------------
        // Small text helpers
        // ---------------------------------------------------------
        std::string ToUtf8(const std::wstring& w) {
            if (w.empty()) return std::string();
            int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
            std::string s(n, '\0');
            WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
            return s;
        }

        std::wstring FromUtf8(const std::string& s) {
            if (s.empty()) return std::wstring();
            int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
            std::wstring w(n, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
            return w;
        }

        std::string Trim(const std::string& s) {
            size_t a = 0, b = s.size();
            while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
            while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
            return s.substr(a, b - a);
        }

        bool HexToBytes(const std::wstring& hex, std::vector<BYTE>& out) {
            out.clear();
            if (hex.size() % 2 != 0) return false;
            auto val = [](wchar_t c) -> int {
                if (c >= L'0' && c <= L'9') return c - L'0';
                if (c >= L'a' && c <= L'f') return c - L'a' + 10;
                if (c >= L'A' && c <= L'F') return c - L'A' + 10;
                return -1;
            };
            for (size_t i = 0; i < hex.size(); i += 2) {
                int hi = val(hex[i]), lo = val(hex[i + 1]);
                if (hi < 0 || lo < 0) return false;
                out.push_back((BYTE)(hi * 16 + lo));
            }
            return true;
        }

        std::string BytesToHex(const BYTE* p, size_t n) {
            static const char* d = "0123456789abcdef";
            std::string s;
            for (size_t i = 0; i < n; ++i) { s += d[p[i] >> 4]; s += d[p[i] & 15]; }
            return s;
        }

        bool Base64Decode(const std::string& in, std::vector<BYTE>& out) {
            out.clear();
            int val = 0, bits = -8;
            for (unsigned char c : in) {
                int d;
                if (c >= 'A' && c <= 'Z') d = c - 'A';
                else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
                else if (c >= '0' && c <= '9') d = c - '0' + 52;
                else if (c == '+') d = 62;
                else if (c == '/') d = 63;
                else if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
                else return false;
                val = (val << 6) | d;
                bits += 6;
                if (bits >= 0) {
                    out.push_back((BYTE)((val >> bits) & 0xFF));
                    bits -= 8;
                }
            }
            return true;
        }

        // ---------------------------------------------------------
        // Crypto (Windows' built-in CNG: SHA-256 + ECDSA P-256)
        // ---------------------------------------------------------
        bool Sha256Bytes(const BYTE* data, size_t len, BYTE out[32]) {
            BCRYPT_ALG_HANDLE alg = nullptr;
            BCRYPT_HASH_HANDLE h = nullptr;
            bool ok = false;
            if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
                if (BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) >= 0) {
                    if (BCryptHashData(h, (PUCHAR)data, (ULONG)len, 0) >= 0 &&
                        BCryptFinishHash(h, out, 32, 0) >= 0) ok = true;
                    BCryptDestroyHash(h);
                }
                BCryptCloseAlgorithmProvider(alg, 0);
            }
            return ok;
        }

        bool Sha256File(const std::wstring& path, std::string& hexOut) {
            HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (f == INVALID_HANDLE_VALUE) return false;

            BCRYPT_ALG_HANDLE alg = nullptr;
            BCRYPT_HASH_HANDLE h = nullptr;
            bool ok = false;
            if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
                if (BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) >= 0) {
                    std::vector<BYTE> buf(65536);
                    bool good = true;
                    for (;;) {
                        DWORD got = 0;
                        if (!ReadFile(f, buf.data(), (DWORD)buf.size(), &got, nullptr)) { good = false; break; }
                        if (got == 0) break;
                        if (BCryptHashData(h, buf.data(), got, 0) < 0) { good = false; break; }
                    }
                    BYTE digest[32];
                    if (good && BCryptFinishHash(h, digest, 32, 0) >= 0) {
                        hexOut = BytesToHex(digest, 32);
                        ok = true;
                    }
                    BCryptDestroyHash(h);
                }
                BCryptCloseAlgorithmProvider(alg, 0);
            }
            CloseHandle(f);
            return ok;
        }

        // The signature is a raw 64-byte ECDSA P-256 signature (r || s)
        // over SHA-256(message). The public key is 64 bytes (X || Y).
        bool VerifySignature(const std::string& message, const std::vector<BYTE>& sig) {
            std::vector<BYTE> pub;
            if (!HexToBytes(UPDATE_PUBLIC_KEY_HEX, pub) || pub.size() != 64) return false;
            if (sig.size() != 64) return false;

            BYTE hash[32];
            if (!Sha256Bytes((const BYTE*)message.data(), message.size(), hash)) return false;

            BCRYPT_ALG_HANDLE alg = nullptr;
            if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) < 0) return false;

            std::vector<BYTE> blob(sizeof(BCRYPT_ECCKEY_BLOB) + 64);
            BCRYPT_ECCKEY_BLOB* hdr = (BCRYPT_ECCKEY_BLOB*)blob.data();
            hdr->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
            hdr->cbKey = 32;
            memcpy(blob.data() + sizeof(BCRYPT_ECCKEY_BLOB), pub.data(), 64);

            bool ok = false;
            BCRYPT_KEY_HANDLE key = nullptr;
            if (BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
                                    blob.data(), (ULONG)blob.size(), 0) >= 0) {
                ok = BCryptVerifySignature(key, nullptr, hash, 32, (PUCHAR)sig.data(), 64, 0) >= 0;
                BCryptDestroyKey(key);
            }
            BCryptCloseAlgorithmProvider(alg, 0);
            return ok;
        }

        // ---------------------------------------------------------
        // HTTPS (WinHTTP)
        // ---------------------------------------------------------
        struct UrlParts {
            std::wstring host;
            std::wstring path;
            INTERNET_PORT port = 443;
        };

        bool CrackHttpsUrl(const std::wstring& url, UrlParts& out) {
            wchar_t host[256]{}, path[2048]{}, extra[1024]{};
            URL_COMPONENTSW uc{};
            uc.dwStructSize = sizeof(uc);
            uc.lpszHostName = host;     uc.dwHostNameLength = 256;
            uc.lpszUrlPath = path;      uc.dwUrlPathLength = 2048;
            uc.lpszExtraInfo = extra;   uc.dwExtraInfoLength = 1024;
            if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;
            if (uc.nScheme != INTERNET_SCHEME_HTTPS) return false;
            out.host = host;
            out.path = std::wstring(path) + extra;
            out.port = uc.nPort;
            return !out.host.empty();
        }

        struct WinHttpHandle {
            HINTERNET h = nullptr;
            ~WinHttpHandle() { if (h) WinHttpCloseHandle(h); }
        };

        bool HttpFetch(const UrlParts& u, unsigned long long maxBytes,
                       const std::function<bool(const BYTE*, DWORD)>& sink) {
            WinHttpHandle session, connect, request;

            session.h = WinHttpOpen(L"ZimoRecorder-Updater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session.h) return false;
            WinHttpSetTimeouts(session.h, 10000, 10000, 15000, 30000);

            connect.h = WinHttpConnect(session.h, u.host.c_str(), u.port, 0);
            if (!connect.h) return false;

            request.h = WinHttpOpenRequest(connect.h, L"GET", u.path.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           WINHTTP_FLAG_SECURE);
            if (!request.h) return false;

            const wchar_t* headers = L"Cache-Control: no-cache\r\n";
            if (!WinHttpSendRequest(request.h, headers, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) return false;
            if (!WinHttpReceiveResponse(request.h, nullptr)) return false;

            DWORD status = 0, size = sizeof(status);
            if (!WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                     WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX)) return false;
            if (status != 200) return false;

            unsigned long long total = 0;
            std::vector<BYTE> buf(65536);
            for (;;) {
                DWORD got = 0;
                if (!WinHttpReadData(request.h, buf.data(), (DWORD)buf.size(), &got)) return false;
                if (got == 0) break;
                total += got;
                if (total > maxBytes) return false;
                if (!sink(buf.data(), got)) return false;
            }
            return true;
        }

        // ---------------------------------------------------------
        // Versions
        // ---------------------------------------------------------
        bool ParseVersion(const std::wstring& s, int v[3]) {
            v[0] = v[1] = v[2] = 0;
            int idx = 0;
            const wchar_t* p = s.c_str();
            while (*p && idx < 3) {
                if (*p < L'0' || *p > L'9') return false;
                wchar_t* end = nullptr;
                v[idx++] = (int)wcstol(p, &end, 10);
                p = end;
                if (*p == L'.') ++p; else break;
            }
            return *p == L'\0' && idx >= 1;
        }

        bool IsNewer(const std::wstring& remote, const std::wstring& local) {
            int a[3], b[3];
            if (!ParseVersion(remote, a) || !ParseVersion(local, b)) return false;
            for (int i = 0; i < 3; ++i) {
                if (a[i] != b[i]) return a[i] > b[i];
            }
            return false;
        }

        // ---------------------------------------------------------
        // Registry (reminder policy)
        // ---------------------------------------------------------
        bool RegReadString(const wchar_t* name, std::wstring& out) {
            wchar_t buf[128]{};
            DWORD size = sizeof(buf);
            if (RegGetValueW(HKEY_CURRENT_USER, kRegKey, name, RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS) return false;
            out = buf;
            return true;
        }

        void RegWriteString(const wchar_t* name, const std::wstring& v) {
            RegSetKeyValueW(HKEY_CURRENT_USER, kRegKey, name, REG_SZ, v.c_str(),
                            (DWORD)((v.size() + 1) * sizeof(wchar_t)));
        }

        bool RegReadDword(const wchar_t* name, DWORD& out) {
            DWORD size = sizeof(out);
            return RegGetValueW(HKEY_CURRENT_USER, kRegKey, name, RRF_RT_REG_DWORD, nullptr, &out, &size) == ERROR_SUCCESS;
        }

        void RegWriteDword(const wchar_t* name, DWORD v) {
            RegSetKeyValueW(HKEY_CURRENT_USER, kRegKey, name, REG_DWORD, &v, sizeof(v));
        }

        long long TodayNumber() { return (long long)(time(nullptr) / 86400); }

        // ---------------------------------------------------------
        // The check itself (runs on a background thread)
        // ---------------------------------------------------------
        std::string UnescapeNotes(const std::string& s) {
            std::string out;
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') { out += '\n'; ++i; }
                else out += s[i];
            }
            return out;
        }

        void CheckBlocking(CheckResult& r) {
            r.status = Status::Failed;

            if (wcslen(UPDATE_PUBLIC_KEY_HEX) != 128) { r.status = Status::NotConfigured; return; }

            UrlParts u;
            u.host = UPDATE_HOST;
            u.path = UPDATE_MANIFEST_PATH;
            u.port = 443;

            std::string body;
            if (!HttpFetch(u, 64 * 1024, [&](const BYTE* p, DWORD n) { body.append((const char*)p, n); return true; })) return;

            // key=value lines
            std::map<std::string, std::string> kv;
            size_t pos = 0;
            while (pos <= body.size()) {
                size_t nl = body.find('\n', pos);
                std::string line = Trim(body.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
                if (!line.empty() && line[0] != '#') {
                    size_t eq = line.find('=');
                    if (eq != std::string::npos) kv[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
                }
                if (nl == std::string::npos) break;
                pos = nl + 1;
            }

            const std::string version = kv["version"];
            const std::string url     = kv["url"];
            std::string sha           = kv["sha256"];
            const std::string sigB64  = kv["signature"];
            if (version.empty() || url.empty() || sha.size() != 64 || sigB64.empty()) return;
            for (char& c : sha) if (c >= 'A' && c <= 'F') c = (char)(c - 'A' + 'a');

            // The installer must come from our own host, over HTTPS
            UrlParts dl;
            if (!CrackHttpsUrl(FromUtf8(url), dl)) return;
            if (_wcsicmp(dl.host.c_str(), UPDATE_HOST) != 0) return;

            // Check the signature on the manifest
            std::vector<BYTE> sig;
            if (!Base64Decode(sigB64, sig)) return;
            std::string message = "ZIMO-UPDATE-v1\n" + version + "\n" + url + "\n" + sha;
            if (!VerifySignature(message, sig)) return;

            r.version = FromUtf8(version);
            r.url     = FromUtf8(url);
            r.sha256  = FromUtf8(sha);

            // "What's new" text: current language first, English as fallback
            std::string notesKey = "notes." + ToUtf8(lang::Code());
            if (kv.find(notesKey) == kv.end()) notesKey = "notes.en";
            auto it = kv.find(notesKey);
            if (it != kv.end()) r.notes = FromUtf8(UnescapeNotes(it->second));

            r.status = IsNewer(r.version, APP_VERSION) ? Status::Available : Status::UpToDate;
        }

        struct CheckCtx { HWND hwnd; UINT msg; bool user; };

        DWORD WINAPI CheckThread(LPVOID p) {
            std::unique_ptr<CheckCtx> c((CheckCtx*)p);
            CheckResult* r = new CheckResult();
            r->userInitiated = c->user;
            CheckBlocking(*r);
            if (!PostMessageW(c->hwnd, c->msg, 0, (LPARAM)r)) delete r;
            return 0;
        }

        struct DownloadCtx { HWND hwnd; UINT msg; CheckResult info; };

        DWORD WINAPI DownloadThread(LPVOID p) {
            std::unique_ptr<DownloadCtx> c((DownloadCtx*)p);
            DownloadResult* d = new DownloadResult();

            wchar_t tmp[MAX_PATH]{};
            GetTempPathW(MAX_PATH, tmp);
            std::wstring path = std::wstring(tmp) + L"ZimoRecorder-update.exe";

            UrlParts u;
            if (CrackHttpsUrl(c->info.url, u) && _wcsicmp(u.host.c_str(), UPDATE_HOST) == 0) {
                HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, nullptr);
                if (f != INVALID_HANDLE_VALUE) {
                    bool fetched = HttpFetch(u, 200ull * 1024 * 1024, [&](const BYTE* p2, DWORD n) {
                        DWORD written = 0;
                        return WriteFile(f, p2, n, &written, nullptr) && written == n;
                    });
                    CloseHandle(f);

                    std::string actual;
                    if (fetched && Sha256File(path, actual) &&
                        actual == ToUtf8(c->info.sha256)) {
                        d->ok = true;
                        d->path = path;
                    } else {
                        DeleteFileW(path.c_str());
                    }
                }
            }
            if (!PostMessageW(c->hwnd, c->msg, 0, (LPARAM)d)) delete d;
            return 0;
        }

    } // namespace

    // -------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------
    void CheckAsync(HWND hwnd, UINT msg, bool userInitiated) {
        CheckCtx* c = new CheckCtx{ hwnd, msg, userInitiated };
        HANDLE h = CreateThread(nullptr, 0, CheckThread, c, 0, nullptr);
        if (h) CloseHandle(h); else delete c;
    }

    void DownloadAsync(HWND hwnd, UINT msg, const CheckResult& r) {
        DownloadCtx* c = new DownloadCtx{ hwnd, msg, r };
        HANDLE h = CreateThread(nullptr, 0, DownloadThread, c, 0, nullptr);
        if (h) CloseHandle(h); else delete c;
    }

    bool IsMandatory(const std::wstring& version) {
        std::wstring seen;
        DWORD day = 0;
        if (!RegReadString(L"FirstSeenVersion", seen) || seen != version || !RegReadDword(L"FirstSeenDay", day)) {
            RegWriteString(L"FirstSeenVersion", version);
            RegWriteDword(L"FirstSeenDay", (DWORD)TodayNumber());
            return UPDATE_MANDATORY_AFTER_DAYS <= 0;
        }
        long long days = TodayNumber() - (long long)day;
        return days >= UPDATE_MANDATORY_AFTER_DAYS;
    }

    bool ShouldPromptToday() {
        DWORD last = 0;
        if (!RegReadDword(L"LastPromptDay", last)) return true;
        return (TodayNumber() - (long long)last) >= UPDATE_PROMPT_EVERY_DAYS;
    }

    void MarkPromptedToday() {
        RegWriteDword(L"LastPromptDay", (DWORD)TodayNumber());
    }

    bool LaunchInstaller(const std::wstring& newExePath) {
        wchar_t self[MAX_PATH]{};
        if (GetModuleFileNameW(nullptr, self, MAX_PATH) == 0) return false;

        std::wstring cmd = L"\"" + newExePath + L"\" --apply-update " +
                           std::to_wstring((unsigned long)GetCurrentProcessId()) +
                           L" \"" + self + L"\"";
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) return false;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    bool RunAsUpdateHelperIfRequested(int& exitCode) {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv) return false;

        if (argc < 4 || wcscmp(argv[1], L"--apply-update") != 0) {
            LocalFree(argv);
            return false;
        }

        DWORD pid = (DWORD)_wtoi(argv[2]);
        std::wstring target = argv[3];
        LocalFree(argv);

        exitCode = 1;
        if (target.size() < 5 || _wcsicmp(target.c_str() + target.size() - 4, L".exe") != 0) return true;

        // Wait for the old app to close
        HANDLE old = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (old) {
            WaitForSingleObject(old, 20000);
            CloseHandle(old);
        }

        wchar_t self[MAX_PATH]{};
        GetModuleFileNameW(nullptr, self, MAX_PATH);

        bool copied = false;
        for (int i = 0; i < 40 && !copied; ++i) {
            if (CopyFileW(self, target.c_str(), FALSE)) copied = true;
            else Sleep(500);
        }

        if (!copied) {
            MessageBoxW(nullptr, lang::Upd(UpdStr::InstallFailed), APP_NAME, MB_OK | MB_ICONERROR);
            return true;
        }

        std::wstring cmd = L"\"" + target + L"\"";
        std::vector<wchar_t> buf(cmd.begin(), cmd.end());
        buf.push_back(L'\0');
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        exitCode = 0;
        return true;
    }

} // namespace updater
