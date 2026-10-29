"""Script-independent PLX1 dictionary encoder; values are opaque bytes."""
import struct


def pack(records):
    records = list(records)
    keys = [key for key, _ in records]
    if any(not key for key in keys) or keys != sorted(set(keys)):
        raise ValueError('keys must be nonempty, unique and sorted')
    alphabet = sorted(set(''.join(keys)))
    if len(alphabet) > 256:
        raise ValueError('PLX1 supports at most 256 distinct characters')
    ids = {ch: i for i, ch in enumerate(alphabet)}
    out = bytearray(b'PLX1' + struct.pack('<HI', len(alphabet), len(records)))
    for ch in alphabet:
        encoded = ch.encode('utf-8')
        out.append(len(encoded))
        out.extend(encoded)
    previous = b''
    for i, (key, value) in enumerate(records):
        encoded = bytes(ids[ch] for ch in key)
        prefix = 0
        if i % 32:
            while prefix < min(len(previous), len(encoded)) and previous[prefix] == encoded[prefix]:
                prefix += 1
        if len(encoded) > 65535 or not value:
            raise ValueError('key exceeds 65535 characters or payload is empty')
        out.extend(struct.pack('<HHI', prefix, len(encoded) - prefix, len(value)))
        out.extend(encoded[prefix:])
        out.extend(value)
        previous = encoded
    return bytes(out)
