import struct
import random
import os
from datetime import datetime, timezone

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DIR = os.path.join(BASE_DIR, 'test')

os.makedirs(TEST_DIR, exist_ok=True)

def calculate_checksum(data, skip_offset):
    hash_val = 2166136261
    for i, byte in enumerate(data):
        if i >= skip_offset and i < skip_offset + 4:
            continue
        hash_val ^= byte
        hash_val = (hash_val * 16777619) & 0xFFFFFFFF
    return hash_val

# Database Consts
GAMEDIARY_MAGIC = 0x444D4147
DB_VERSION = 3

# =========================
# 1. Generate Games
# =========================
games = []

for i in range(1, 61):
    uid = i
    game_id = f'ULUS{i:05d}'
    name = f'Test Game {i:02d} Challenge'
    cat = random.choice([0, 1, 2])  # PSP, PS1, Homebrew

    entry = struct.pack(
        '<I16s64s8sB3B',
        uid,
        game_id.encode('ascii'),
        name.encode('utf-8'),
        b'0x120',
        cat,
        0, 0, 0
    )

    games.append(entry)

# Header
header_data = [
    GAMEDIARY_MAGIC,
    DB_VERSION,
    len(games),
    len(games) + 1,
    GAMEDIARY_MAGIC,
    0,
    0,
    0
]

header_bin = bytearray(struct.pack('<8I', *header_data))
checksum = calculate_checksum(header_bin, 20)
struct.pack_into('<I', header_bin, 20, checksum)

with open(os.path.join(TEST_DIR, 'games.dat'), 'wb') as f:
    f.write(header_bin)
    for g in games:
        f.write(g)
    f.write(header_bin)

# =========================
# 2. Generate Sessions
# =========================

sessions = []

# -------- MARCH --------
start_ts = int(datetime(2026, 3, 1, tzinfo=timezone.utc).timestamp())
march_usage = {d: 0 for d in range(31)}

march_sessions = 0
while march_sessions < 1000:
    day = random.randint(0, 30)
    game_uid = random.randint(1, 60)

    duration = random.randint(300, 2700)

    if march_usage[day] + duration > 80000:
        continue

    march_usage[day] += duration

    ts = start_ts + (day * 86400) + random.randint(0, 86400 - duration)

    sessions.append(struct.pack('<3I', game_uid, duration, ts))
    march_sessions += 1

# -------- APRIL --------
april_start_ts = int(datetime(2026, 4, 1, tzinfo=timezone.utc).timestamp())
april_usage = {d: 0 for d in range(30)}

april_sessions = 0
while april_sessions < 1000:
    day = random.randint(0, 29)
    game_uid = random.randint(1, 60)

    duration = random.randint(300, 2700)

    if april_usage[day] + duration > 80000:
        continue

    april_usage[day] += duration

    ts = april_start_ts + (day * 86400) + random.randint(0, 86400 - duration)

    sessions.append(struct.pack('<3I', game_uid, duration, ts))
    april_sessions += 1

sessions.sort(key=lambda x: struct.unpack('<3I', x)[2])

with open(os.path.join(TEST_DIR, 'sessions.dat'), 'wb') as f:
    for s in sessions:
        f.write(s)

# =========================
# 3. Debug / Validation
# =========================

march_count = 0
april_count = 0

for s in sessions:
    _, _, ts = struct.unpack('<3I', s)
    dt = datetime.utcfromtimestamp(ts)

    if dt.month == 3:
        march_count += 1
    elif dt.month == 4:
        april_count += 1

print(f'Generated {len(games)} games and {len(sessions)} sessions.')
print(f'March sessions: {march_count}')
print(f'April sessions: {april_count}')
