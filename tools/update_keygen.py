"""
Run this ONCE. It makes the key pair that protects your updates.

  pip install cryptography
  python update_keygen.py

It creates  update_private_key.pem  (KEEP SECRET - never upload it, never
put it on GitHub) and prints a long PUBLIC key line. Paste that line into
src/Config.h as UPDATE_PUBLIC_KEY_HEX, then rebuild the app.
"""
import os
import sys
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization

PRIVATE = "update_private_key.pem"

if os.path.exists(PRIVATE):
    print(PRIVATE + " already exists - not overwriting it.")
    sys.exit(1)

key = ec.generate_private_key(ec.SECP256R1())
with open(PRIVATE, "wb") as f:
    f.write(key.private_bytes(
        serialization.Encoding.PEM,
        serialization.PrivateFormat.PKCS8,
        serialization.NoEncryption()))

nums = key.public_key().public_numbers()
pub_hex = nums.x.to_bytes(32, "big").hex() + nums.y.to_bytes(32, "big").hex()

print("Saved " + PRIVATE + "  <-- keep it secret and make a backup copy!")
print()
print("Put this line in src/Config.h:")
print()
print('#define UPDATE_PUBLIC_KEY_HEX   L"' + pub_hex + '"')
