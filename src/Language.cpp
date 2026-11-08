#include "Language.h"
#include <string>
#include <cstdio>
#include <cwchar>   // wcscmp, fgetws, fputws

namespace {

    Lang g_lang = Lang::EN;

    const wchar_t* const kEN[] = {
        L"Screenshot",  // MenuCaptureFull
        L"Start Recording",  // MenuRecordStart
        L"Stop Recording",  // MenuRecordStop
        L"Open Save Folder",  // MenuOpenFolder
        L"About",  // MenuAbout
        L"Exit",  // MenuExit
        L"Language",  // MenuLanguage
        L"Screenshot saved.",  // MsgScreenshotSaved
        L"Failed to save screenshot.",  // MsgSaveError
        L"Recording started...",  // MsgRecordStart
        L"Recording stopped and saved.",  // MsgRecordStop
        L"Failed to start recording (ffmpeg.exe not found?)",  // MsgRecordError
        L"ZIMO RECORDER is already running (check the system tray).",  // AlreadyRunning
        L"Screenshot & screen recording tool.",  // AboutIntro
        L"Tray menu actions always use the full screen.",  // AboutMenuHint
        L"Hotkeys:",  // AboutHotkeysTitle
        L"Hotkeys let you pick a region first:",  // AboutHotkeyHint
        L"Version",  // AboutVersionLabel
        L"Screenshot (pick a region)",  // HotkeyScreenshotLabel
        L"Record (pick a region) - press again to stop",  // HotkeyRecordLabel
        L"Record GIF (pick a region) - press again to stop",  // HotkeyGifLabel
        L"Pause",  // IndicatorPause
        L"Resume",  // IndicatorResume
        L"Stop",  // IndicatorStop
        L"Line",  // EditorToolLine
        L"Arrow",  // EditorToolArrow
        L"Text",  // EditorToolText
        L"Blur",  // EditorToolBlur
        L"Undo",  // EditorUndo
        L"Copy",  // EditorCopy
        L"Save",  // EditorSave
        L"Cancel",  // EditorCancel
        L"Screenshot copied to clipboard.",  // MsgCopiedToClipboard
        L"Start GIF Recording",  // MenuGifStart
        L"Stop GIF Recording",  // MenuGifStop
        L"GIF recording started...",  // MsgGifStart
        L"GIF recording stopped and saved.",  // MsgGifStop
        L"Extract Text (OCR)",  // MenuOcr
        L"Text copied to clipboard.",  // MsgOcrCopied
        L"No text was found.",  // MsgOcrNoText
        L"Text recognition failed.",  // MsgOcrError
    };
    static_assert(sizeof(kEN)/sizeof(kEN[0]) == (size_t)StrId::Count, "kEN size mismatch");

    const wchar_t* const kFA[] = {
        L"اسکرین‌شات",  // MenuCaptureFull
        L"شروع ضبط صفحه",  // MenuRecordStart
        L"توقف ضبط",  // MenuRecordStop
        L"باز کردن پوشه ذخیره",  // MenuOpenFolder
        L"درباره",  // MenuAbout
        L"خروج",  // MenuExit
        L"زبان",  // MenuLanguage
        L"اسکرین‌شات ذخیره شد.",  // MsgScreenshotSaved
        L"خطا در ذخیره‌ی اسکرین‌شات.",  // MsgSaveError
        L"ضبط صفحه شروع شد...",  // MsgRecordStart
        L"ضبط متوقف شد و ذخیره گردید.",  // MsgRecordStop
        L"خطا در شروع ضبط (ffmpeg.exe پیدا نشد؟)",  // MsgRecordError
        L"ZIMO RECORDER از قبل در حال اجراست (آیکون آن را در Tray ببینید).",  // AlreadyRunning
        L"ابزار اسکرین‌شات و ضبط صفحه.",  // AboutIntro
        L"گزینه‌های منو همیشه از تمام صفحه عمل می‌کنن.",  // AboutMenuHint
        L"میانبرها:",  // AboutHotkeysTitle
        L"میانبرها اول بهت اجازه می‌دن یه ناحیه انتخاب کنی:",  // AboutHotkeyHint
        L"نسخه",  // AboutVersionLabel
        L"اسکرین‌شات (انتخاب ناحیه)",  // HotkeyScreenshotLabel
        L"ضبط (انتخاب ناحیه) - دوباره بزن تا متوقف بشه",  // HotkeyRecordLabel
        L"ضبط GIF (انتخاب ناحیه) - دوباره بزن تا متوقف بشه",  // HotkeyGifLabel
        L"توقف موقت",  // IndicatorPause
        L"ادامه",  // IndicatorResume
        L"توقف",  // IndicatorStop
        L"خط",  // EditorToolLine
        L"فلش",  // EditorToolArrow
        L"متن",  // EditorToolText
        L"تار کردن",  // EditorToolBlur
        L"برگشت",  // EditorUndo
        L"کپی",  // EditorCopy
        L"ذخیره",  // EditorSave
        L"انصراف",  // EditorCancel
        L"اسکرین‌شات در کلیپ‌بورد کپی شد.",  // MsgCopiedToClipboard
        L"شروع ضبط GIF",  // MenuGifStart
        L"توقف ضبط GIF",  // MenuGifStop
        L"ضبط GIF شروع شد...",  // MsgGifStart
        L"ضبط GIF متوقف شد و ذخیره گردید.",  // MsgGifStop
        L"تشخیص متن (OCR)",  // MenuOcr
        L"متن در کلیپ‌بورد کپی شد.",  // MsgOcrCopied
        L"متنی پیدا نشد.",  // MsgOcrNoText
        L"تشخیص متن ناموفق بود.",  // MsgOcrError
    };
    static_assert(sizeof(kFA)/sizeof(kFA[0]) == (size_t)StrId::Count, "kFA size mismatch");

    const wchar_t* const kDE[] = {
        L"Bildschirmfoto",  // MenuCaptureFull
        L"Aufnahme starten",  // MenuRecordStart
        L"Aufnahme stoppen",  // MenuRecordStop
        L"Speicherordner öffnen",  // MenuOpenFolder
        L"Über",  // MenuAbout
        L"Beenden",  // MenuExit
        L"Sprache",  // MenuLanguage
        L"Screenshot gespeichert.",  // MsgScreenshotSaved
        L"Screenshot konnte nicht gespeichert werden.",  // MsgSaveError
        L"Aufnahme gestartet...",  // MsgRecordStart
        L"Aufnahme gestoppt und gespeichert.",  // MsgRecordStop
        L"Aufnahme konnte nicht gestartet werden (ffmpeg.exe nicht gefunden?)",  // MsgRecordError
        L"ZIMO RECORDER läuft bereits (Symbol in der Taskleiste beachten).",  // AlreadyRunning
        L"Tool für Screenshots und Bildschirmaufnahmen.",  // AboutIntro
        L"Aktionen im Menü verwenden immer den ganzen Bildschirm.",  // AboutMenuHint
        L"Tastenkombinationen:",  // AboutHotkeysTitle
        L"Mit Tastenkombinationen kannst du zuerst einen Bereich auswählen:",  // AboutHotkeyHint
        L"Version",  // AboutVersionLabel
        L"Bildschirmfoto (Bereich auswählen)",  // HotkeyScreenshotLabel
        L"Aufnahme (Bereich auswählen) - erneut drücken zum Stoppen",  // HotkeyRecordLabel
        L"GIF-Aufnahme (Bereich auswählen) - erneut drücken zum Stoppen",  // HotkeyGifLabel
        L"Pause",  // IndicatorPause
        L"Fortsetzen",  // IndicatorResume
        L"Stopp",  // IndicatorStop
        L"Linie",  // EditorToolLine
        L"Pfeil",  // EditorToolArrow
        L"Text",  // EditorToolText
        L"Weichzeichnen",  // EditorToolBlur
        L"Rückgängig",  // EditorUndo
        L"Kopieren",  // EditorCopy
        L"Speichern",  // EditorSave
        L"Schließen",  // EditorCancel
        L"Screenshot in die Zwischenablage kopiert.",  // MsgCopiedToClipboard
        L"GIF-Aufnahme starten",  // MenuGifStart
        L"GIF-Aufnahme stoppen",  // MenuGifStop
        L"GIF-Aufnahme gestartet...",  // MsgGifStart
        L"GIF-Aufnahme gestoppt und gespeichert.",  // MsgGifStop
        L"Text erkennen (OCR)",  // MenuOcr
        L"Text in die Zwischenablage kopiert.",  // MsgOcrCopied
        L"Kein Text gefunden.",  // MsgOcrNoText
        L"Texterkennung fehlgeschlagen.",  // MsgOcrError
    };
    static_assert(sizeof(kDE)/sizeof(kDE[0]) == (size_t)StrId::Count, "kDE size mismatch");

    const wchar_t* const kRU[] = {
        L"Снимок экрана",  // MenuCaptureFull
        L"Начать запись",  // MenuRecordStart
        L"Остановить запись",  // MenuRecordStop
        L"Открыть папку сохранения",  // MenuOpenFolder
        L"О программе",  // MenuAbout
        L"Выход",  // MenuExit
        L"Язык",  // MenuLanguage
        L"Снимок экрана сохранён.",  // MsgScreenshotSaved
        L"Не удалось сохранить снимок экрана.",  // MsgSaveError
        L"Запись начата...",  // MsgRecordStart
        L"Запись остановлена и сохранена.",  // MsgRecordStop
        L"Не удалось начать запись (ffmpeg.exe не найден?)",  // MsgRecordError
        L"ZIMO RECORDER уже запущен (значок в области уведомлений).",  // AlreadyRunning
        L"Инструмент для скриншотов и записи экрана.",  // AboutIntro
        L"Действия в меню всегда используют весь экран.",  // AboutMenuHint
        L"Горячие клавиши:",  // AboutHotkeysTitle
        L"Горячие клавиши сначала дают выбрать область:",  // AboutHotkeyHint
        L"Версия",  // AboutVersionLabel
        L"Снимок экрана (выбрать область)",  // HotkeyScreenshotLabel
        L"Запись (выбрать область) — нажмите снова, чтобы остановить",  // HotkeyRecordLabel
        L"Запись GIF (выбрать область) — нажмите снова, чтобы остановить",  // HotkeyGifLabel
        L"Пауза",  // IndicatorPause
        L"Продолжить",  // IndicatorResume
        L"Стоп",  // IndicatorStop
        L"Линия",  // EditorToolLine
        L"Стрелка",  // EditorToolArrow
        L"Текст",  // EditorToolText
        L"Размытие",  // EditorToolBlur
        L"Отменить",  // EditorUndo
        L"Копировать",  // EditorCopy
        L"Сохранить",  // EditorSave
        L"Отмена",  // EditorCancel
        L"Снимок экрана скопирован в буфер обмена.",  // MsgCopiedToClipboard
        L"Начать запись GIF",  // MenuGifStart
        L"Остановить запись GIF",  // MenuGifStop
        L"Запись GIF начата...",  // MsgGifStart
        L"Запись GIF остановлена и сохранена.",  // MsgGifStop
        L"Распознать текст (OCR)",  // MenuOcr
        L"Текст скопирован в буфер обмена.",  // MsgOcrCopied
        L"Текст не найден.",  // MsgOcrNoText
        L"Не удалось распознать текст.",  // MsgOcrError
    };
    static_assert(sizeof(kRU)/sizeof(kRU[0]) == (size_t)StrId::Count, "kRU size mismatch");

    const wchar_t* const kFR[] = {
        L"Capture d'écran",  // MenuCaptureFull
        L"Démarrer l'enregistrement",  // MenuRecordStart
        L"Arrêter l'enregistrement",  // MenuRecordStop
        L"Ouvrir le dossier de sauvegarde",  // MenuOpenFolder
        L"À propos",  // MenuAbout
        L"Quitter",  // MenuExit
        L"Langue",  // MenuLanguage
        L"Capture d'écran enregistrée.",  // MsgScreenshotSaved
        L"Échec de l'enregistrement de la capture.",  // MsgSaveError
        L"Enregistrement démarré...",  // MsgRecordStart
        L"Enregistrement arrêté et sauvegardé.",  // MsgRecordStop
        L"Échec du démarrage de l'enregistrement (ffmpeg.exe introuvable ?)",  // MsgRecordError
        L"ZIMO RECORDER est déjà en cours d'exécution (voir l'icône dans la barre des tâches).",  // AlreadyRunning
        L"Outil de capture d'écran et d'enregistrement.",  // AboutIntro
        L"Les actions du menu utilisent toujours l'écran entier.",  // AboutMenuHint
        L"Raccourcis clavier :",  // AboutHotkeysTitle
        L"Les raccourcis vous laissent d'abord choisir une zone :",  // AboutHotkeyHint
        L"Version",  // AboutVersionLabel
        L"Capture d'écran (choisir une zone)",  // HotkeyScreenshotLabel
        L"Enregistrer (choisir une zone) - appuyez à nouveau pour arrêter",  // HotkeyRecordLabel
        L"Enregistrer en GIF (choisir une zone) - appuyez à nouveau pour arrêter",  // HotkeyGifLabel
        L"Pause",  // IndicatorPause
        L"Reprendre",  // IndicatorResume
        L"Arrêter",  // IndicatorStop
        L"Ligne",  // EditorToolLine
        L"Flèche",  // EditorToolArrow
        L"Texte",  // EditorToolText
        L"Flou",  // EditorToolBlur
        L"Annuler",  // EditorUndo
        L"Copier",  // EditorCopy
        L"Enregistrer",  // EditorSave
        L"Fermer",  // EditorCancel
        L"Capture d'écran copiée dans le presse-papiers.",  // MsgCopiedToClipboard
        L"Démarrer l'enregistrement GIF",  // MenuGifStart
        L"Arrêter l'enregistrement GIF",  // MenuGifStop
        L"Enregistrement GIF démarré...",  // MsgGifStart
        L"Enregistrement GIF arrêté et sauvegardé.",  // MsgGifStop
        L"Extraire le texte (OCR)",  // MenuOcr
        L"Texte copié dans le presse-papiers.",  // MsgOcrCopied
        L"Aucun texte trouvé.",  // MsgOcrNoText
        L"Échec de la reconnaissance de texte.",  // MsgOcrError
    };
    static_assert(sizeof(kFR)/sizeof(kFR[0]) == (size_t)StrId::Count, "kFR size mismatch");

    const wchar_t* const kES[] = {
        L"Captura de pantalla",  // MenuCaptureFull
        L"Iniciar grabación",  // MenuRecordStart
        L"Detener grabación",  // MenuRecordStop
        L"Abrir carpeta de guardado",  // MenuOpenFolder
        L"Acerca de",  // MenuAbout
        L"Salir",  // MenuExit
        L"Idioma",  // MenuLanguage
        L"Captura de pantalla guardada.",  // MsgScreenshotSaved
        L"No se pudo guardar la captura.",  // MsgSaveError
        L"Grabación iniciada...",  // MsgRecordStart
        L"Grabación detenida y guardada.",  // MsgRecordStop
        L"No se pudo iniciar la grabación (¿no se encontró ffmpeg.exe?)",  // MsgRecordError
        L"ZIMO RECORDER ya se está ejecutando (revisa el icono en la bandeja).",  // AlreadyRunning
        L"Herramienta de capturas y grabación de pantalla.",  // AboutIntro
        L"Las acciones del menú siempre usan la pantalla completa.",  // AboutMenuHint
        L"Atajos de teclado:",  // AboutHotkeysTitle
        L"Los atajos te permiten elegir primero una región:",  // AboutHotkeyHint
        L"Versión",  // AboutVersionLabel
        L"Captura de pantalla (elegir una región)",  // HotkeyScreenshotLabel
        L"Grabar (elegir una región) - pulsa de nuevo para detener",  // HotkeyRecordLabel
        L"Grabar GIF (elegir una región) - pulsa de nuevo para detener",  // HotkeyGifLabel
        L"Pausa",  // IndicatorPause
        L"Reanudar",  // IndicatorResume
        L"Detener",  // IndicatorStop
        L"Línea",  // EditorToolLine
        L"Flecha",  // EditorToolArrow
        L"Texto",  // EditorToolText
        L"Desenfoque",  // EditorToolBlur
        L"Deshacer",  // EditorUndo
        L"Copiar",  // EditorCopy
        L"Guardar",  // EditorSave
        L"Cancelar",  // EditorCancel
        L"Captura de pantalla copiada al portapapeles.",  // MsgCopiedToClipboard
        L"Iniciar grabación GIF",  // MenuGifStart
        L"Detener grabación GIF",  // MenuGifStop
        L"Grabación GIF iniciada...",  // MsgGifStart
        L"Grabación GIF detenida y guardada.",  // MsgGifStop
        L"Extraer texto (OCR)",  // MenuOcr
        L"Texto copiado al portapapeles.",  // MsgOcrCopied
        L"No se encontró texto.",  // MsgOcrNoText
        L"Error al reconocer el texto.",  // MsgOcrError
    };
    static_assert(sizeof(kES)/sizeof(kES[0]) == (size_t)StrId::Count, "kES size mismatch");

    const wchar_t* const kTR[] = {
        L"Ekran Görüntüsü",  // MenuCaptureFull
        L"Kaydı Başlat",  // MenuRecordStart
        L"Kaydı Durdur",  // MenuRecordStop
        L"Kayıt Klasörünü Aç",  // MenuOpenFolder
        L"Hakkında",  // MenuAbout
        L"Çıkış",  // MenuExit
        L"Dil",  // MenuLanguage
        L"Ekran görüntüsü kaydedildi.",  // MsgScreenshotSaved
        L"Ekran görüntüsü kaydedilemedi.",  // MsgSaveError
        L"Kayıt başladı...",  // MsgRecordStart
        L"Kayıt durduruldu ve kaydedildi.",  // MsgRecordStop
        L"Kayıt başlatılamadı (ffmpeg.exe bulunamadı mı?)",  // MsgRecordError
        L"ZIMO RECORDER zaten çalışıyor (bildirim alanındaki simgeye bakın).",  // AlreadyRunning
        L"Ekran görüntüsü ve ekran kaydı aracı.",  // AboutIntro
        L"Menü işlemleri her zaman tüm ekranı kullanır.",  // AboutMenuHint
        L"Kısayollar:",  // AboutHotkeysTitle
        L"Kısayollar önce bir bölge seçmene izin verir:",  // AboutHotkeyHint
        L"Sürüm",  // AboutVersionLabel
        L"Ekran Görüntüsü (bölge seç)",  // HotkeyScreenshotLabel
        L"Kayıt (bölge seç) - durdurmak için tekrar bas",  // HotkeyRecordLabel
        L"GIF Kaydı (bölge seç) - durdurmak için tekrar bas",  // HotkeyGifLabel
        L"Duraklat",  // IndicatorPause
        L"Devam Et",  // IndicatorResume
        L"Durdur",  // IndicatorStop
        L"Çizgi",  // EditorToolLine
        L"Ok",  // EditorToolArrow
        L"Metin",  // EditorToolText
        L"Bulanıklaştır",  // EditorToolBlur
        L"Geri Al",  // EditorUndo
        L"Kopyala",  // EditorCopy
        L"Kaydet",  // EditorSave
        L"İptal",  // EditorCancel
        L"Ekran görüntüsü panoya kopyalandı.",  // MsgCopiedToClipboard
        L"GIF Kaydını Başlat",  // MenuGifStart
        L"GIF Kaydını Durdur",  // MenuGifStop
        L"GIF kaydı başladı...",  // MsgGifStart
        L"GIF kaydı durduruldu ve kaydedildi.",  // MsgGifStop
        L"Metni Tanı (OCR)",  // MenuOcr
        L"Metin panoya kopyalandı.",  // MsgOcrCopied
        L"Metin bulunamadı.",  // MsgOcrNoText
        L"Metin tanıma başarısız oldu.",  // MsgOcrError
    };
    static_assert(sizeof(kTR)/sizeof(kTR[0]) == (size_t)StrId::Count, "kTR size mismatch");

    const wchar_t* const kAR[] = {
        L"لقطة شاشة",  // MenuCaptureFull
        L"بدء التسجيل",  // MenuRecordStart
        L"إيقاف التسجيل",  // MenuRecordStop
        L"فتح مجلد الحفظ",  // MenuOpenFolder
        L"حول البرنامج",  // MenuAbout
        L"خروج",  // MenuExit
        L"اللغة",  // MenuLanguage
        L"تم حفظ لقطة الشاشة.",  // MsgScreenshotSaved
        L"فشل حفظ لقطة الشاشة.",  // MsgSaveError
        L"بدأ التسجيل...",  // MsgRecordStart
        L"تم إيقاف التسجيل وحفظه.",  // MsgRecordStop
        L"فشل بدء التسجيل (هل ffmpeg.exe غير موجود؟)",  // MsgRecordError
        L"البرنامج ZIMO RECORDER يعمل بالفعل (تحقق من الأيقونة في شريط المهام).",  // AlreadyRunning
        L"أداة لالتقاط الشاشة وتسجيلها.",  // AboutIntro
        L"إجراءات القائمة تستخدم دائمًا الشاشة كاملة.",  // AboutMenuHint
        L"اختصارات لوحة المفاتيح:",  // AboutHotkeysTitle
        L"الاختصارات تتيح لك اختيار منطقة أولاً:",  // AboutHotkeyHint
        L"الإصدار",  // AboutVersionLabel
        L"لقطة شاشة (اختيار منطقة)",  // HotkeyScreenshotLabel
        L"تسجيل (اختيار منطقة) - اضغط مجددًا للإيقاف",  // HotkeyRecordLabel
        L"تسجيل GIF (اختيار منطقة) - اضغط مجددًا للإيقاف",  // HotkeyGifLabel
        L"إيقاف مؤقت",  // IndicatorPause
        L"استئناف",  // IndicatorResume
        L"إيقاف",  // IndicatorStop
        L"خط",  // EditorToolLine
        L"سهم",  // EditorToolArrow
        L"نص",  // EditorToolText
        L"تمويه",  // EditorToolBlur
        L"تراجع",  // EditorUndo
        L"نسخ",  // EditorCopy
        L"حفظ",  // EditorSave
        L"إلغاء",  // EditorCancel
        L"تم نسخ لقطة الشاشة إلى الحافظة.",  // MsgCopiedToClipboard
        L"بدء تسجيل GIF",  // MenuGifStart
        L"إيقاف تسجيل GIF",  // MenuGifStop
        L"بدأ تسجيل GIF...",  // MsgGifStart
        L"تم إيقاف تسجيل GIF وحفظه.",  // MsgGifStop
        L"استخراج النص (OCR)",  // MenuOcr
        L"تم نسخ النص إلى الحافظة.",  // MsgOcrCopied
        L"لم يتم العثور على نص.",  // MsgOcrNoText
        L"فشل التعرف على النص.",  // MsgOcrError
    };
    static_assert(sizeof(kAR)/sizeof(kAR[0]) == (size_t)StrId::Count, "kAR size mismatch");

    const wchar_t* const kZH[] = {
        L"截图",  // MenuCaptureFull
        L"开始录制",  // MenuRecordStart
        L"停止录制",  // MenuRecordStop
        L"打开保存文件夹",  // MenuOpenFolder
        L"关于",  // MenuAbout
        L"退出",  // MenuExit
        L"语言",  // MenuLanguage
        L"截图已保存。",  // MsgScreenshotSaved
        L"截图保存失败。",  // MsgSaveError
        L"录制已开始...",  // MsgRecordStart
        L"录制已停止并保存。",  // MsgRecordStop
        L"无法开始录制(未找到 ffmpeg.exe?)",  // MsgRecordError
        L"ZIMO RECORDER 已在运行(请查看系统托盘图标)。",  // AlreadyRunning
        L"屏幕截图与录制工具。",  // AboutIntro
        L"菜单操作始终使用整个屏幕。",  // AboutMenuHint
        L"快捷键:",  // AboutHotkeysTitle
        L"快捷键会让你先选择一个区域:",  // AboutHotkeyHint
        L"版本",  // AboutVersionLabel
        L"截图(选择区域)",  // HotkeyScreenshotLabel
        L"录制(选择区域) - 再次按下以停止",  // HotkeyRecordLabel
        L"GIF 录制(选择区域) - 再次按下以停止",  // HotkeyGifLabel
        L"暂停",  // IndicatorPause
        L"继续",  // IndicatorResume
        L"停止",  // IndicatorStop
        L"直线",  // EditorToolLine
        L"箭头",  // EditorToolArrow
        L"文字",  // EditorToolText
        L"模糊",  // EditorToolBlur
        L"撤销",  // EditorUndo
        L"复制",  // EditorCopy
        L"保存",  // EditorSave
        L"取消",  // EditorCancel
        L"截图已复制到剪贴板。",  // MsgCopiedToClipboard
        L"开始录制 GIF",  // MenuGifStart
        L"停止录制 GIF",  // MenuGifStop
        L"GIF 录制已开始...",  // MsgGifStart
        L"GIF 录制已停止并保存。",  // MsgGifStop
        L"提取文字 (OCR)",  // MenuOcr
        L"文字已复制到剪贴板。",  // MsgOcrCopied
        L"未找到文字。",  // MsgOcrNoText
        L"文字识别失败。",  // MsgOcrError
    };
    static_assert(sizeof(kZH)/sizeof(kZH[0]) == (size_t)StrId::Count, "kZH size mismatch");

    const wchar_t* const kIT[] = {
        L"Screenshot",  // MenuCaptureFull
        L"Avvia registrazione",  // MenuRecordStart
        L"Interrompi registrazione",  // MenuRecordStop
        L"Apri cartella di salvataggio",  // MenuOpenFolder
        L"Informazioni",  // MenuAbout
        L"Esci",  // MenuExit
        L"Lingua",  // MenuLanguage
        L"Screenshot salvato.",  // MsgScreenshotSaved
        L"Impossibile salvare lo screenshot.",  // MsgSaveError
        L"Registrazione avviata...",  // MsgRecordStart
        L"Registrazione interrotta e salvata.",  // MsgRecordStop
        L"Impossibile avviare la registrazione (ffmpeg.exe non trovato?)",  // MsgRecordError
        L"ZIMO RECORDER è già in esecuzione (controlla l'icona nella barra delle applicazioni).",  // AlreadyRunning
        L"Strumento di cattura schermo e registrazione.",  // AboutIntro
        L"Le azioni del menu usano sempre lo schermo intero.",  // AboutMenuHint
        L"Scorciatoie:",  // AboutHotkeysTitle
        L"Le scorciatoie ti permettono di scegliere prima un'area:",  // AboutHotkeyHint
        L"Versione",  // AboutVersionLabel
        L"Screenshot (scegli un'area)",  // HotkeyScreenshotLabel
        L"Registra (scegli un'area) - premi di nuovo per interrompere",  // HotkeyRecordLabel
        L"Registra GIF (scegli un'area) - premi di nuovo per interrompere",  // HotkeyGifLabel
        L"Pausa",  // IndicatorPause
        L"Riprendi",  // IndicatorResume
        L"Interrompi",  // IndicatorStop
        L"Linea",  // EditorToolLine
        L"Freccia",  // EditorToolArrow
        L"Testo",  // EditorToolText
        L"Sfoca",  // EditorToolBlur
        L"Annulla",  // EditorUndo
        L"Copia",  // EditorCopy
        L"Salva",  // EditorSave
        L"Chiudi",  // EditorCancel
        L"Screenshot copiato negli appunti.",  // MsgCopiedToClipboard
        L"Avvia registrazione GIF",  // MenuGifStart
        L"Interrompi registrazione GIF",  // MenuGifStop
        L"Registrazione GIF avviata...",  // MsgGifStart
        L"Registrazione GIF interrotta e salvata.",  // MsgGifStop
        L"Estrai testo (OCR)",  // MenuOcr
        L"Testo copiato negli appunti.",  // MsgOcrCopied
        L"Nessun testo trovato.",  // MsgOcrNoText
        L"Riconoscimento del testo non riuscito.",  // MsgOcrError
    };
    static_assert(sizeof(kIT)/sizeof(kIT[0]) == (size_t)StrId::Count, "kIT size mismatch");

    const wchar_t* const kPT[] = {
        L"Captura de Tela",  // MenuCaptureFull
        L"Iniciar Gravação",  // MenuRecordStart
        L"Parar Gravação",  // MenuRecordStop
        L"Abrir Pasta de Salvamento",  // MenuOpenFolder
        L"Sobre",  // MenuAbout
        L"Sair",  // MenuExit
        L"Idioma",  // MenuLanguage
        L"Captura de tela salva.",  // MsgScreenshotSaved
        L"Falha ao salvar a captura de tela.",  // MsgSaveError
        L"Gravação iniciada...",  // MsgRecordStart
        L"Gravação interrompida e salva.",  // MsgRecordStop
        L"Falha ao iniciar a gravação (ffmpeg.exe não encontrado?)",  // MsgRecordError
        L"O ZIMO RECORDER já está em execução (verifique o ícone na bandeja do sistema).",  // AlreadyRunning
        L"Ferramenta de captura de tela e gravação.",  // AboutIntro
        L"As ações do menu sempre usam a tela inteira.",  // AboutMenuHint
        L"Atalhos:",  // AboutHotkeysTitle
        L"Os atalhos deixam você escolher uma região primeiro:",  // AboutHotkeyHint
        L"Versão",  // AboutVersionLabel
        L"Captura de Tela (escolher uma região)",  // HotkeyScreenshotLabel
        L"Gravar (escolher uma região) - pressione novamente para parar",  // HotkeyRecordLabel
        L"Gravar GIF (escolher uma região) - pressione novamente para parar",  // HotkeyGifLabel
        L"Pausar",  // IndicatorPause
        L"Retomar",  // IndicatorResume
        L"Parar",  // IndicatorStop
        L"Linha",  // EditorToolLine
        L"Seta",  // EditorToolArrow
        L"Texto",  // EditorToolText
        L"Desfocar",  // EditorToolBlur
        L"Desfazer",  // EditorUndo
        L"Copiar",  // EditorCopy
        L"Salvar",  // EditorSave
        L"Cancelar",  // EditorCancel
        L"Captura de tela copiada para a área de transferência.",  // MsgCopiedToClipboard
        L"Iniciar Gravação GIF",  // MenuGifStart
        L"Parar Gravação GIF",  // MenuGifStop
        L"Gravação GIF iniciada...",  // MsgGifStart
        L"Gravação GIF interrompida e salva.",  // MsgGifStop
        L"Extrair Texto (OCR)",  // MenuOcr
        L"Texto copiado para a área de transferência.",  // MsgOcrCopied
        L"Nenhum texto encontrado.",  // MsgOcrNoText
        L"Falha no reconhecimento de texto.",  // MsgOcrError
    };
    static_assert(sizeof(kPT)/sizeof(kPT[0]) == (size_t)StrId::Count, "kPT size mismatch");

    const wchar_t* const kJA[] = {
        L"スクリーンショット",  // MenuCaptureFull
        L"録画を開始",  // MenuRecordStart
        L"録画を停止",  // MenuRecordStop
        L"保存フォルダーを開く",  // MenuOpenFolder
        L"このアプリについて",  // MenuAbout
        L"終了",  // MenuExit
        L"言語",  // MenuLanguage
        L"スクリーンショットを保存しました。",  // MsgScreenshotSaved
        L"スクリーンショットの保存に失敗しました。",  // MsgSaveError
        L"録画を開始しました...",  // MsgRecordStart
        L"録画を停止して保存しました。",  // MsgRecordStop
        L"録画を開始できませんでした(ffmpeg.exe が見つかりません)",  // MsgRecordError
        L"ZIMO RECORDER はすでに実行中です(タスクトレイのアイコンを確認してください)。",  // AlreadyRunning
        L"スクリーンショットと画面録画ツール。",  // AboutIntro
        L"メニューの操作は常に画面全体を使用します。",  // AboutMenuHint
        L"ショートカットキー:",  // AboutHotkeysTitle
        L"ショートカットキーでは先に範囲を選択できます:",  // AboutHotkeyHint
        L"バージョン",  // AboutVersionLabel
        L"スクリーンショット(範囲を選択)",  // HotkeyScreenshotLabel
        L"録画(範囲を選択) - もう一度押すと停止します",  // HotkeyRecordLabel
        L"GIF録画(範囲を選択) - もう一度押すと停止します",  // HotkeyGifLabel
        L"一時停止",  // IndicatorPause
        L"再開",  // IndicatorResume
        L"停止",  // IndicatorStop
        L"直線",  // EditorToolLine
        L"矢印",  // EditorToolArrow
        L"テキスト",  // EditorToolText
        L"ぼかし",  // EditorToolBlur
        L"元に戻す",  // EditorUndo
        L"コピー",  // EditorCopy
        L"保存",  // EditorSave
        L"キャンセル",  // EditorCancel
        L"スクリーンショットをクリップボードにコピーしました。",  // MsgCopiedToClipboard
        L"GIF録画を開始",  // MenuGifStart
        L"GIF録画を停止",  // MenuGifStop
        L"GIF録画を開始しました...",  // MsgGifStart
        L"GIF録画を停止して保存しました。",  // MsgGifStop
        L"テキストを抽出(OCR)",  // MenuOcr
        L"テキストをクリップボードにコピーしました。",  // MsgOcrCopied
        L"テキストが見つかりませんでした。",  // MsgOcrNoText
        L"テキスト認識に失敗しました。",  // MsgOcrError
    };
    static_assert(sizeof(kJA)/sizeof(kJA[0]) == (size_t)StrId::Count, "kJA size mismatch");

    const wchar_t* const kKO[] = {
        L"스크린샷",  // MenuCaptureFull
        L"녹화 시작",  // MenuRecordStart
        L"녹화 중지",  // MenuRecordStop
        L"저장 폴더 열기",  // MenuOpenFolder
        L"정보",  // MenuAbout
        L"종료",  // MenuExit
        L"언어",  // MenuLanguage
        L"스크린샷이 저장되었습니다.",  // MsgScreenshotSaved
        L"스크린샷 저장에 실패했습니다.",  // MsgSaveError
        L"녹화가 시작되었습니다...",  // MsgRecordStart
        L"녹화가 중지되고 저장되었습니다.",  // MsgRecordStop
        L"녹화를 시작할 수 없습니다 (ffmpeg.exe를 찾을 수 없음?)",  // MsgRecordError
        L"ZIMO RECORDER가 이미 실행 중입니다 (시스템 트레이 아이콘을 확인하세요).",  // AlreadyRunning
        L"스크린샷 및 화면 녹화 도구.",  // AboutIntro
        L"메뉴 작업은 항상 전체 화면을 사용합니다.",  // AboutMenuHint
        L"단축키:",  // AboutHotkeysTitle
        L"단축키는 먼저 영역을 선택하게 해줍니다:",  // AboutHotkeyHint
        L"버전",  // AboutVersionLabel
        L"스크린샷 (영역 선택)",  // HotkeyScreenshotLabel
        L"녹화 (영역 선택) - 다시 누르면 중지",  // HotkeyRecordLabel
        L"GIF 녹화 (영역 선택) - 다시 누르면 중지",  // HotkeyGifLabel
        L"일시정지",  // IndicatorPause
        L"재개",  // IndicatorResume
        L"중지",  // IndicatorStop
        L"선",  // EditorToolLine
        L"화살표",  // EditorToolArrow
        L"텍스트",  // EditorToolText
        L"흐림 효과",  // EditorToolBlur
        L"실행 취소",  // EditorUndo
        L"복사",  // EditorCopy
        L"저장",  // EditorSave
        L"취소",  // EditorCancel
        L"스크린샷이 클립보드에 복사되었습니다.",  // MsgCopiedToClipboard
        L"GIF 녹화 시작",  // MenuGifStart
        L"GIF 녹화 중지",  // MenuGifStop
        L"GIF 녹화가 시작되었습니다...",  // MsgGifStart
        L"GIF 녹화가 중지되고 저장되었습니다.",  // MsgGifStop
        L"텍스트 추출 (OCR)",  // MenuOcr
        L"텍스트가 클립보드에 복사되었습니다.",  // MsgOcrCopied
        L"텍스트를 찾을 수 없습니다.",  // MsgOcrNoText
        L"텍스트 인식에 실패했습니다.",  // MsgOcrError
    };
    static_assert(sizeof(kKO)/sizeof(kKO[0]) == (size_t)StrId::Count, "kKO size mismatch");

    const wchar_t* const kHI[] = {
        L"स्क्रीनशॉट",  // MenuCaptureFull
        L"रिकॉर्डिंग शुरू करें",  // MenuRecordStart
        L"रिकॉर्डिंग बंद करें",  // MenuRecordStop
        L"सेव फ़ोल्डर खोलें",  // MenuOpenFolder
        L"ऐप के बारे में",  // MenuAbout
        L"बाहर निकलें",  // MenuExit
        L"भाषा",  // MenuLanguage
        L"स्क्रीनशॉट सेव हो गया।",  // MsgScreenshotSaved
        L"स्क्रीनशॉट सेव करने में विफल।",  // MsgSaveError
        L"रिकॉर्डिंग शुरू हुई...",  // MsgRecordStart
        L"रिकॉर्डिंग बंद हो गई और सेव हो गई।",  // MsgRecordStop
        L"रिकॉर्डिंग शुरू नहीं हो सकी (ffmpeg.exe नहीं मिला?)",  // MsgRecordError
        L"ZIMO RECORDER पहले से चल रहा है (सिस्टम ट्रे आइकन देखें)।",  // AlreadyRunning
        L"स्क्रीनशॉट और स्क्रीन रिकॉर्डिंग टूल।",  // AboutIntro
        L"मेनू क्रियाएं हमेशा पूरी स्क्रीन का उपयोग करती हैं।",  // AboutMenuHint
        L"शॉर्टकट:",  // AboutHotkeysTitle
        L"शॉर्टकट पहले आपको एक क्षेत्र चुनने देते हैं:",  // AboutHotkeyHint
        L"संस्करण",  // AboutVersionLabel
        L"स्क्रीनशॉट (क्षेत्र चुनें)",  // HotkeyScreenshotLabel
        L"रिकॉर्ड करें (क्षेत्र चुनें) - रोकने के लिए फिर से दबाएं",  // HotkeyRecordLabel
        L"GIF रिकॉर्ड करें (क्षेत्र चुनें) - रोकने के लिए फिर से दबाएं",  // HotkeyGifLabel
        L"रोकें",  // IndicatorPause
        L"फिर से शुरू करें",  // IndicatorResume
        L"बंद करें",  // IndicatorStop
        L"रेखा",  // EditorToolLine
        L"तीर",  // EditorToolArrow
        L"टेक्स्ट",  // EditorToolText
        L"धुंधला करें",  // EditorToolBlur
        L"पूर्ववत करें",  // EditorUndo
        L"कॉपी करें",  // EditorCopy
        L"सेव करें",  // EditorSave
        L"रद्द करें",  // EditorCancel
        L"स्क्रीनशॉट क्लिपबोर्ड में कॉपी हो गया।",  // MsgCopiedToClipboard
        L"GIF रिकॉर्डिंग शुरू करें",  // MenuGifStart
        L"GIF रिकॉर्डिंग बंद करें",  // MenuGifStop
        L"GIF रिकॉर्डिंग शुरू हुई...",  // MsgGifStart
        L"GIF रिकॉर्डिंग बंद हो गई और सेव हो गई।",  // MsgGifStop
        L"टेक्स्ट निकालें (OCR)",  // MenuOcr
        L"टेक्स्ट क्लिपबोर्ड में कॉपी हो गया।",  // MsgOcrCopied
        L"कोई टेक्स्ट नहीं मिला।",  // MsgOcrNoText
        L"टेक्स्ट पहचान विफल रही।",  // MsgOcrError
    };
    static_assert(sizeof(kHI)/sizeof(kHI[0]) == (size_t)StrId::Count, "kHI size mismatch");

    const wchar_t* const kNL[] = {
        L"Schermafbeelding",  // MenuCaptureFull
        L"Opname starten",  // MenuRecordStart
        L"Opname stoppen",  // MenuRecordStop
        L"Opslagmap openen",  // MenuOpenFolder
        L"Over",  // MenuAbout
        L"Afsluiten",  // MenuExit
        L"Taal",  // MenuLanguage
        L"Schermafbeelding opgeslagen.",  // MsgScreenshotSaved
        L"Schermafbeelding opslaan mislukt.",  // MsgSaveError
        L"Opname gestart...",  // MsgRecordStart
        L"Opname gestopt en opgeslagen.",  // MsgRecordStop
        L"Opname starten mislukt (ffmpeg.exe niet gevonden?)",  // MsgRecordError
        L"ZIMO RECORDER draait al (controleer het pictogram in het systeemvak).",  // AlreadyRunning
        L"Hulpmiddel voor schermafbeeldingen en schermopnames.",  // AboutIntro
        L"Menuacties gebruiken altijd het volledige scherm.",  // AboutMenuHint
        L"Sneltoetsen:",  // AboutHotkeysTitle
        L"Met sneltoetsen kies je eerst een gebied:",  // AboutHotkeyHint
        L"Versie",  // AboutVersionLabel
        L"Schermafbeelding (gebied kiezen)",  // HotkeyScreenshotLabel
        L"Opnemen (gebied kiezen) - druk nogmaals om te stoppen",  // HotkeyRecordLabel
        L"GIF opnemen (gebied kiezen) - druk nogmaals om te stoppen",  // HotkeyGifLabel
        L"Pauzeren",  // IndicatorPause
        L"Hervatten",  // IndicatorResume
        L"Stoppen",  // IndicatorStop
        L"Lijn",  // EditorToolLine
        L"Pijl",  // EditorToolArrow
        L"Tekst",  // EditorToolText
        L"Vervagen",  // EditorToolBlur
        L"Ongedaan maken",  // EditorUndo
        L"Kopiëren",  // EditorCopy
        L"Opslaan",  // EditorSave
        L"Annuleren",  // EditorCancel
        L"Schermafbeelding gekopieerd naar klembord.",  // MsgCopiedToClipboard
        L"GIF-opname starten",  // MenuGifStart
        L"GIF-opname stoppen",  // MenuGifStop
        L"GIF-opname gestart...",  // MsgGifStart
        L"GIF-opname gestopt en opgeslagen.",  // MsgGifStop
        L"Tekst extraheren (OCR)",  // MenuOcr
        L"Tekst gekopieerd naar klembord.",  // MsgOcrCopied
        L"Geen tekst gevonden.",  // MsgOcrNoText
        L"Tekstherkenning mislukt.",  // MsgOcrError
    };
    static_assert(sizeof(kNL)/sizeof(kNL[0]) == (size_t)StrId::Count, "kNL size mismatch");

    const wchar_t* const* const kTables[(int)Lang::Count] = {
        kEN,
        kFA,
        kDE,
        kRU,
        kFR,
        kES,
        kTR,
        kAR,
        kZH,
        kIT,
        kPT,
        kJA,
        kKO,
        kHI,
        kNL,
    };

    const wchar_t* LangToCode(Lang l) {
        switch (l) {
        case Lang::EN: return L"en";
        case Lang::FA: return L"fa";
        case Lang::DE: return L"de";
        case Lang::RU: return L"ru";
        case Lang::FR: return L"fr";
        case Lang::ES: return L"es";
        case Lang::TR: return L"tr";
        case Lang::AR: return L"ar";
        case Lang::ZH: return L"zh";
        case Lang::IT: return L"it";
        case Lang::PT: return L"pt";
        case Lang::JA: return L"ja";
        case Lang::KO: return L"ko";
        case Lang::HI: return L"hi";
        case Lang::NL: return L"nl";
        default: return L"en";
        }
    }

    Lang CodeToLang(const wchar_t* code) {
        if (wcscmp(code, L"en") == 0) return Lang::EN;
        if (wcscmp(code, L"fa") == 0) return Lang::FA;
        if (wcscmp(code, L"de") == 0) return Lang::DE;
        if (wcscmp(code, L"ru") == 0) return Lang::RU;
        if (wcscmp(code, L"fr") == 0) return Lang::FR;
        if (wcscmp(code, L"es") == 0) return Lang::ES;
        if (wcscmp(code, L"tr") == 0) return Lang::TR;
        if (wcscmp(code, L"ar") == 0) return Lang::AR;
        if (wcscmp(code, L"zh") == 0) return Lang::ZH;
        if (wcscmp(code, L"it") == 0) return Lang::IT;
        if (wcscmp(code, L"pt") == 0) return Lang::PT;
        if (wcscmp(code, L"ja") == 0) return Lang::JA;
        if (wcscmp(code, L"ko") == 0) return Lang::KO;
        if (wcscmp(code, L"hi") == 0) return Lang::HI;
        if (wcscmp(code, L"nl") == 0) return Lang::NL;
        return Lang::EN;
    }

    std::wstring SettingsFilePath() {
        wchar_t exeDir[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
        if (len == 0 || len == MAX_PATH) return L"zimorecorder-lang.cfg";

        std::wstring path(exeDir);
        size_t slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return L"zimorecorder-lang.cfg";

        return path.substr(0, slash + 1) + L"zimorecorder-lang.cfg";
    }

} // namespace

namespace lang {

    void Load() {
        g_lang = Lang::EN; // default on first run
        std::wstring path = SettingsFilePath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"r") == 0 && f) {
            wchar_t buf[16]{};
            if (fgetws(buf, 16, f)) {
                for (wchar_t* p = buf; *p; ++p) {
                    if (*p == L'\r' || *p == L'\n') { *p = 0; break; }
                }
                g_lang = CodeToLang(buf);
            }
            fclose(f);
        }
    }

    Lang Current() { return g_lang; }

    const wchar_t* Code() { return LangToCode(g_lang); }

    void Set(Lang l) {
        g_lang = l;
        std::wstring path = SettingsFilePath();
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"w") == 0 && f) {
            fputws(LangToCode(l), f);
            fclose(f);
        }
    }

    const wchar_t* Str(StrId id) {
        return kTables[(int)g_lang][(int)id];
    }

    const wchar_t* NativeName(Lang l) {
        switch (l) {
        case Lang::EN: return L"English";
        case Lang::FA: return L"فارسی";
        case Lang::DE: return L"Deutsch";
        case Lang::RU: return L"Русский";
        case Lang::FR: return L"Français";
        case Lang::ES: return L"Español";
        case Lang::TR: return L"Türkçe";
        case Lang::AR: return L"العربية";
        case Lang::ZH: return L"中文";
        case Lang::IT: return L"Italiano";
        case Lang::PT: return L"Português";
        case Lang::JA: return L"日本語";
        case Lang::KO: return L"한국어";
        case Lang::HI: return L"हिन्दी";
        case Lang::NL: return L"Nederlands";
        default: return L"English";
        }
    }

} // namespace lang
