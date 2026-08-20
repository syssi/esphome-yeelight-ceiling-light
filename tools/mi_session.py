"""Shared Xiaomi cloud login for the tools here: cached sessions and QR login.

Two problems this solves.

A Xiaomi API session is just three values - userId, ssecurity and serviceToken -
and the extractor keeps them in memory only, so every invocation logged in from
scratch and paid for it with a captcha and an emailed 2FA code. They are cached
here instead and reused until the server rejects them, which turns the normal
case into no login at all.

Password login also depends on that emailed code arriving, which is both slow and
rate limited. QR login has neither a captcha nor a 2FA step: it long-polls while
the code is scanned in the Mi Home app. Use --qr when the mail does not show up.

The cache holds live credentials, so it is written 0600 and must never be
committed. Delete it to force a fresh login.
"""

import json
import os
import pathlib
import sys

# Set MI_STATE_DIR to put these somewhere else. The captcha and QR images land
# here too, because the extractor's own options - an HTTP server on :31415 or a
# desktop image viewer - are both useless over SSH.
STATE_DIR = pathlib.Path(os.environ.get(
    "MI_STATE_DIR", pathlib.Path.home() / ".cache" / "esphome-yeelight"))
SESSION_PATH = STATE_DIR / "mi_session.json"
IDENTITY_PATH = STATE_DIR / "mi_identity.json"
CAPTCHA_PATH = STATE_DIR / "captcha.jpg"
QR_PATH = STATE_DIR / "mi_qr.png"

_FIELDS = ("userId", "_ssecurity", "_serviceToken", "_agent", "_device_id")


def load_extractor(extractor_path: str, qr: bool = False):
    """Import token_extractor and route its image prompts to a readable file.

    The extractor serves the captcha or QR code over HTTP on :31415, or opens an
    image viewer. Neither is reachable over SSH, so write the bytes to a fixed
    path instead.
    """
    sys.path.insert(0, extractor_path)
    try:
        import token_extractor as te  # noqa: PLC0415
    except ImportError:
        print(f"could not import token_extractor from {extractor_path}",
              file=sys.stderr)
        raise SystemExit(2)

    target = QR_PATH if qr else CAPTCHA_PATH

    def save_image(image_content, **_kwargs):
        STATE_DIR.mkdir(parents=True, exist_ok=True)
        target.write_bytes(image_content)
        print(f"\n*** {'QR code' if qr else 'captcha'} written to {target} ***\n",
              flush=True)

    te.present_image_image = save_image
    return te


def _identity(te) -> dict:
    """A stable device fingerprint, created once and reused.

    The extractor generates a random User-Agent and deviceId per instance, and the
    deviceId goes into a cookie that Xiaomi uses to recognise a device. So every
    login presented itself as brand-new hardware, which plausibly explains why
    each one demanded an emailed 2FA code and why we eventually hit "sent too many
    codes". Pinning both makes later logins look like the same device.

    This is a hypothesis about Xiaomi's side, not something verified here.
    """
    if IDENTITY_PATH.is_file():
        try:
            return json.loads(IDENTITY_PATH.read_text())
        except (OSError, ValueError):
            pass
    ident = {"_agent": te.XiaomiCloudConnector.generate_agent(),
             "_device_id": te.XiaomiCloudConnector.generate_device_id()}
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    IDENTITY_PATH.write_text(json.dumps(ident))
    IDENTITY_PATH.chmod(0o600)
    print(f"created a stable device identity in {IDENTITY_PATH}", flush=True)
    return ident


def _save(connector) -> None:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    data = {f: getattr(connector, f) for f in _FIELDS}
    SESSION_PATH.write_text(json.dumps(data))
    SESSION_PATH.chmod(0o600)


def _restore(connector) -> bool:
    if not SESSION_PATH.is_file():
        return False
    try:
        data = json.loads(SESSION_PATH.read_text())
    except (OSError, ValueError):
        return False
    if not all(data.get(f) for f in ("userId", "_ssecurity", "_serviceToken")):
        return False
    for f in _FIELDS:
        if data.get(f):
            setattr(connector, f, data[f])
    return True


def get_connector(te, server: str, qr: bool = False):
    """A logged-in connector, reusing a cached session when it still works."""
    ident = _identity(te)

    cached = te.PasswordXiaomiCloudConnector()
    for k, v in ident.items():
        setattr(cached, k, v)
    if _restore(cached):
        # Cheapest authenticated call available, used purely as a liveness probe.
        probe = cached.get_homes(server)
        if probe and probe.get("result"):
            print("reusing cached session - no captcha or 2FA needed", flush=True)
            return cached
        print("cached session rejected, logging in again", flush=True)

    connector = (te.QrCodeXiaomiCloudConnector() if qr
                 else te.PasswordXiaomiCloudConnector())
    # Before login: the deviceId cookie is set during the first login step.
    for k, v in ident.items():
        setattr(connector, k, v)
    if not connector.login():
        return None
    _save(connector)
    print(f"session cached in {SESSION_PATH}", flush=True)
    return connector
