#!/usr/bin/env python3
"""
Décodeur Meshtastic en temps réel depuis les ports ZMQ du flowgraph GNU Radio.
"""

import zmq
import threading
import datetime
import struct
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
import base64

# Clé AES par défaut Meshtastic canal LongFast
DEFAULT_KEY_B64 = "1PG7OiApB1nwvP+rz05pAQ=="
DEFAULT_KEY = base64.b64decode(DEFAULT_KEY_B64)

CANAUX = [
    (20000, 'SF7  BW250k'),
    (20001, 'SF8  BW250k'),
    (20002, 'SF9  BW250k'),
    (20003, 'SF10 BW250k'),
    (20004, 'SF11 BW250k [LongFast]'),
    (20005, 'SF11 BW125k'),
    (20006, 'SF12 BW125k'),
    (20007, 'SF12 BW62k'),
]

def decrypt_meshtastic(payload, packet_id, sender, key=DEFAULT_KEY):
    try:
        nonce = struct.pack('<II', packet_id, sender) + b'\x00' * 8
        cipher = Cipher(algorithms.AES(key), modes.CTR(nonce), backend=default_backend())
        dec = cipher.decryptor()
        return dec.update(payload) + dec.finalize()
    except Exception:
        return None

def parse_portnum(portnum):
    ports = {
        1:  'TEXT_MESSAGE',
        3:  'POSITION',
        4:  'NODEINFO',
        67: 'TELEMETRY',
        71: 'TRACEROUTE',
        73: 'NEIGHBORINFO',
    }
    return ports.get(portnum, f'PORT_{portnum}')

def decode_data(decrypted):
    try:
        from meshtastic.protobuf import mesh_pb2
        data = mesh_pb2.Data()
        data.ParseFromString(decrypted)
        portnum  = data.portnum
        payload  = data.payload
        portname = parse_portnum(portnum)

        if portnum == 1:
            try:
                return portname, payload.decode('utf-8')
            except:
                return portname, payload.hex()
        elif portnum == 3:
            pos = mesh_pb2.Position()
            pos.ParseFromString(payload)
            return portname, f"lat={pos.latitude_i/1e7:.6f} lon={pos.longitude_i/1e7:.6f} alt={pos.altitude}m"
        elif portnum == 4:
            user = mesh_pb2.User()
            user.ParseFromString(payload)
            return portname, f"{user.long_name} ({user.short_name})"
        else:
            return portname, payload.hex()
    except Exception as e:
        # Essayer de lire comme texte brut
        try:
            texte = decrypted.decode('utf-8')
            if texte.isprintable():
                return 'TEXT_RAW', texte
        except:
            pass
        return 'RAW', decrypted.hex()

def parse_packet(raw, canal):
    now = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    if len(raw) < 8:
        return

    # Header Meshtastic dans le payload LoRa :
    # bytes 0-3  : destination (LE)
    # bytes 4-7  : source (LE)
    # bytes 8-11 : packet_id (LE)
    # bytes 12   : flags
    # bytes 13   : channel hash
    # bytes 14+  : payload AES chiffré

    dst       = struct.unpack('<I', raw[0:4])[0]
    src       = struct.unpack('<I', raw[4:8])[0]
    packet_id = struct.unpack('<I', raw[8:12])[0] if len(raw) >= 12 else 0
    flags     = raw[12] if len(raw) > 12 else 0
    channel   = raw[13] if len(raw) > 13 else 0
    encrypted = raw[14:] if len(raw) > 14 else raw[8:]

    dst_str = f"{dst:08x}"
    src_str = f"{src:08x}"

    print(f"\nDatetime: {now}")
    print(f"##### PACKET DATA START #####")
    print(f"dest {dst_str}  sender {src_str}")
    print(f"packet_id {packet_id:#010x}  flags {flags}  channel {channel}")
    print(f"canal: {canal}  raw: {raw.hex()}")

    # Déchiffrement AES-CTR
    decrypted = decrypt_meshtastic(encrypted, packet_id, src)
    if decrypted:
        portname, content = decode_data(decrypted)
        print(f"AES key: {DEFAULT_KEY_B64}")
        print(f"decrypted: {decrypted.hex()}")
        print(f"##### PACKET DATA END #####")
        print(f">>> {portname}  {src_str} -> {dst_str}  |  {content}")
    else:
        print(f"[déchiffrement impossible]")
        print(f"##### PACKET DATA END #####")


def ecouter(port, nom):
    ctx = zmq.Context()
    sock = ctx.socket(zmq.SUB)
    sock.connect(f'tcp://localhost:{port}')
    sock.setsockopt(zmq.SUBSCRIBE, b'')
    sock.setsockopt(zmq.RCVTIMEO, 1000)
    print(f"[OK] Ecoute {nom} sur port {port}")
    while True:
        try:
            msg = sock.recv()
            if msg:
                parse_packet(msg, nom)
        except zmq.Again:
            pass
        except Exception as e:
            print(f"[{nom}] Erreur: {e}")


def main():
    print("=" * 60)
    print("  DECODEUR MESHTASTIC - GNU Radio CaribouLite")
    print("=" * 60)
    print(f"Cle AES: {DEFAULT_KEY_B64}")
    print("Frequence : 869.525 MHz | LongFast SF11 BW250kHz")
    print("=" * 60)

    threads = []
    for port, nom in CANAUX:
        t = threading.Thread(target=ecouter, args=(port, nom), daemon=True)
        t.start()
        threads.append(t)

    print("\nEn attente de paquets Meshtastic... (Ctrl+C pour quitter)\n")
    try:
        for t in threads:
            t.join()
    except KeyboardInterrupt:
        print("\nArret.")


if __name__ == '__main__':
    main()
