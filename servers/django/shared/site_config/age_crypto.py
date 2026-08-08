import subprocess
from pathlib import Path

from shared.http.exceptions import AppError


class AgeCrypto:
    def __init__(self, site_key_path, site_pub_path, dev_pub_path):
        self.site_key_path = Path(site_key_path)
        self.site_pub_path = Path(site_pub_path)
        self.dev_pub_path = Path(dev_pub_path)

    def require_site_key(self):
        if not self.site_key_path.is_file():
            raise AppError(
                f"Missing site age key: {self.site_key_path}",
                status_code=500,
            )

    def require_site_pub(self):
        if not self.site_pub_path.is_file():
            raise AppError(
                f"Missing site age public key: {self.site_pub_path}",
                status_code=500,
            )

    def require_dev_pub(self):
        if not self.dev_pub_path.is_file():
            raise AppError(
                f"Missing vendor age public key: {self.dev_pub_path}",
                status_code=500,
            )

    def seal_to_recipient(self, plaintext, recipient_pub_path):
        recipient = Path(recipient_pub_path)
        if not recipient.is_file():
            raise AppError(f"Missing age public key: {recipient}", status_code=500)
        completed = subprocess.run(
            ["age", "-R", str(recipient)],
            input=plaintext,
            capture_output=True,
            check=False,
        )
        ciphertext = completed.stdout
        if completed.returncode != 0:
            err = completed.stderr.decode("utf-8", errors="replace").strip()
            raise AppError(f"age seal failed: {err or 'unknown error'}", status_code=500)
        return ciphertext

    def seal_to_site(self, plaintext):
        self.require_site_pub()
        return self.seal_to_recipient(plaintext, self.site_pub_path)

    def seal_to_vendor(self, plaintext):
        self.require_dev_pub()
        return self.seal_to_recipient(plaintext, self.dev_pub_path)

    def open_with_site_key(self, ciphertext):
        self.require_site_key()
        completed = subprocess.run(
            ["age", "-d", "-i", str(self.site_key_path)],
            input=ciphertext,
            capture_output=True,
            check=False,
        )
        plaintext = completed.stdout
        if completed.returncode != 0:
            raise AppError("Failed to decrypt age package", status_code=400)
        return plaintext
