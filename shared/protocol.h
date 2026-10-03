#pragma once
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

constexpr auto MSG_AUTH                  = "auth";
constexpr auto MSG_PING                  = "ping";
constexpr auto MSG_ONLINE                = "online";
constexpr auto MSG_SCREEN_SIZE           = "screen_size";
constexpr auto MSG_SET_CAPTURE_INTERVAL  = "set_capture_interval";

constexpr auto MSG_AUTH_OK      = "auth_ok";
constexpr auto MSG_AUTH_FAIL    = "auth_fail";
constexpr auto MSG_PONG         = "pong";
constexpr auto MSG_START_STREAM = "start_stream";
constexpr auto MSG_STOP_STREAM  = "stop_stream";

constexpr auto MSG_AUDIO_START_CAPTURE = "audio_start_capture";
constexpr auto MSG_AUDIO_STOP_CAPTURE  = "audio_stop_capture";
constexpr auto MSG_AUDIO_START_PLAY    = "audio_start_play";
constexpr auto MSG_AUDIO_STOP_PLAY     = "audio_stop_play";
constexpr auto MSG_AUDIO_CAPTURE_DATA  = "audio_capture_data";
constexpr auto MSG_AUDIO_PLAYBACK_DATA = "audio_playback_data";
constexpr auto MSG_TEXT_MESSAGE        = "text_message";
constexpr auto MSG_TEXT_MESSAGE_REPLY  = "text_message_reply";
constexpr auto MSG_SIGNAL              = "signal";
constexpr auto MSG_SIGNAL_ACK          = "signal_ack";
constexpr auto MSG_SET_STREAM_QUALITY  = "set_stream_quality";
constexpr auto MSG_RUN_COMMAND         = "run_command";

constexpr auto MSG_MOUSE_MOVE    = "mouse_move";
constexpr auto MSG_MOUSE_CLICK   = "mouse_click";
constexpr auto MSG_MOUSE_SCROLL  = "mouse_scroll";
constexpr auto MSG_KEYBOARD_KEY  = "keyboard_key";
constexpr auto MSG_KEYBOARD_TEXT = "keyboard_text";

constexpr auto MSG_CLIPBOARD_TEXT    = "clipboard_text";
constexpr auto MSG_REQUEST_CLIPBOARD = "request_clipboard";

constexpr auto MSG_CAMERA_START = "camera_start";
constexpr auto MSG_CAMERA_STOP  = "camera_stop";
constexpr auto MSG_CAMERA_FRAME = "camera_frame";

constexpr char BIN_SCREEN_FRAME   = 0x01;
constexpr char BIN_AUDIO_CAPTURE  = 0x02;
constexpr char BIN_AUDIO_PLAYBACK = 0x03;
constexpr char BIN_CAMERA_FRAME   = 0x04;

constexpr int MAX_TEXT_MESSAGE_BYTES = 64 * 1024;

inline QByteArray makeMessage(const QString &type, QJsonObject payload = {}) {
    payload["type"] = type;
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

inline QJsonObject parseMessage(const QString &raw) {
    const QByteArray utf8 = raw.toUtf8();
    if (utf8.isEmpty() || utf8.size() > MAX_TEXT_MESSAGE_BYTES)
        return {};
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(utf8, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};
    return doc.object();
}

inline QByteArray makeSetCaptureIntervalMessage(int intervalMs) {
    QJsonObject payload;
    payload["intervalMs"] = intervalMs;
    return makeMessage(MSG_SET_CAPTURE_INTERVAL, payload);
}

inline QByteArray makeSignalMessage(const QString &color, const QString &text = {}) {
    QJsonObject payload;
    payload["color"] = color;
    if (!text.isEmpty()) payload["text"] = text;
    return makeMessage(MSG_SIGNAL, payload);
}

inline QByteArray makeTextMessage(const QString &targetHostname, const QString &text) {
    QJsonObject payload;
    payload["to"]   = targetHostname;
    payload["text"] = text;
    return makeMessage(MSG_TEXT_MESSAGE, payload);
}

inline QByteArray makeSetQualityMessage(int maxWidth, int quality) {
    QJsonObject payload;
    payload["maxWidth"] = maxWidth;
    payload["quality"]  = quality;
    return makeMessage(MSG_SET_STREAM_QUALITY, payload);
}

inline QByteArray makeMouseMoveMessage(int x, int y, bool absolute = true) {
    QJsonObject payload;
    payload["x"] = x;
    payload["y"] = y;
    payload["absolute"] = absolute;
    return makeMessage(MSG_MOUSE_MOVE, payload);
}

inline QByteArray makeMouseClickMessage(int button, bool press, bool release) {
    QJsonObject payload;
    payload["button"]  = button;
    payload["press"]   = press;
    payload["release"] = release;
    return makeMessage(MSG_MOUSE_CLICK, payload);
}

inline QByteArray makeMouseScrollMessage(int delta) {
    QJsonObject payload;
    payload["delta"] = delta;
    return makeMessage(MSG_MOUSE_SCROLL, payload);
}

inline QByteArray makeKeyboardKeyMessage(int keyCode, bool press, bool release) {
    QJsonObject payload;
    payload["key"]     = keyCode;
    payload["press"]   = press;
    payload["release"] = release;
    return makeMessage(MSG_KEYBOARD_KEY, payload);
}

inline QByteArray makeKeyboardTextMessage(const QString &text) {
    QJsonObject payload;
    payload["text"] = text;
    return makeMessage(MSG_KEYBOARD_TEXT, payload);
}

inline QByteArray makeScreenSizeMessage(int width, int height) {
    QJsonObject payload;
    payload["width"]  = width;
    payload["height"] = height;
    return makeMessage(MSG_SCREEN_SIZE, payload);
}

inline QByteArray makeClipboardTextMessage(const QString &text) {
    QJsonObject payload;
    payload["text"] = text;
    return makeMessage(MSG_CLIPBOARD_TEXT, payload);
}
