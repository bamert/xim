import binascii
from pyblake2 import blake2b

hash = blake2b("This is just a test!", digest_size=32).digest();
print(hash)
print("hash:"+binascii.hexlify(hash))
