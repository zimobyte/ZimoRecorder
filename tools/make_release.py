"""
Makes the files you upload to your server for a new version.

  python make_release.py --exe path\\to\\ZimoRecorder.exe --version 1.1.0 ^
         --notes-en "Faster recording\\nNew editor tools" ^
         --notes-de "Schnellere Aufnahme\\nNeue Editor-Werkzeuge"

Add  --notes-xx  for any other language code (de, ru, fr, es, tr, ar, zh,
it, pt, ja, ko, hi, nl). Languages without notes show the English notes.
Use  \\n  inside the text for a new line.

Output folder  release/  contains:
   ZimoRecorder-<version>.exe   and   latest.txt
Upload BOTH to your server folder (the one that matches UPDATE_MANIFEST_PATH
and --base-url). Upload latest.txt LAST - that is what makes the update live.
"""
import argparse
import base64
import hashlib
import os
import shutil

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature

LANGS = ["en", "fa", "de", "ru", "fr", "es", "tr", "ar", "zh", "it", "pt", "ja", "ko", "hi", "nl"]

p = argparse.ArgumentParser()
p.add_argument("--exe", required=True)
p.add_argument("--version", required=True, help="for example 1.1.0")
p.add_argument("--base-url", default="https://updates.nettfreedaily.xyz/zimo/",
               help="folder URL on your server that will hold the exe")
p.add_argument("--private-key", default="update_private_key.pem")
for l in LANGS:
    p.add_argument("--notes-" + l, default="")
a = p.parse_args()

base = a.base_url if a.base_url.endswith("/") else a.base_url + "/"
exe_name = "ZimoRecorder-%s.exe" % a.version
url = base + exe_name

with open(a.exe, "rb") as f:
    sha = hashlib.sha256(f.read()).hexdigest()

with open(a.private_key, "rb") as f:
    key = serialization.load_pem_private_key(f.read(), password=None)

# Must match the app (Updater.cpp):  "ZIMO-UPDATE-v1\n" version \n url \n sha256
message = ("ZIMO-UPDATE-v1\n%s\n%s\n%s" % (a.version, url, sha)).encode("utf-8")
der = key.sign(message, ec.ECDSA(hashes.SHA256()))
r, s = decode_dss_signature(der)
signature = base64.b64encode(r.to_bytes(32, "big") + s.to_bytes(32, "big")).decode()

os.makedirs("release", exist_ok=True)
shutil.copyfile(a.exe, os.path.join("release", exe_name))

lines = ["version=" + a.version, "url=" + url, "sha256=" + sha, "signature=" + signature]
for l in LANGS:
    text = getattr(a, "notes_" + l)
    if text:
        lines.append("notes.%s=%s" % (l, text.replace("\r", "").replace("\n", "\\n")))

with open(os.path.join("release", "latest.txt"), "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines) + "\n")

print("Done. Upload these two files from the release folder:")
print("  " + exe_name)
print("  latest.txt   (upload this one LAST)")
