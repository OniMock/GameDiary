"""
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
"""

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
# Game Name Pool by Language
# =========================

TITLES_EN = [
    "Shadow of the Colossus", "Dragon's Lair", "Crimson Skies", "Iron Fist",
    "Frozen Abyss", "Silent Storm", "Burning Crusade", "Lost Kingdom",
    "Phantom Edge", "Steel Wings", "Dark Horizon", "Neon Requiem",
    "Vault Runners", "Dust and Glory", "Broken Throne", "Echo Protocol",
    "Rusted Crown", "Amber Siege", "Crystal Depths", "Void Sentinel",
    "Ashen Path", "Hollow Knight", "Rogue Circuit", "Ember Falls",
    "Iron Tide", "Last Bastion", "Shattered Realm", "Pale Wanderer",
    "Storm Reaver", "Binary Ghost", "Obsidian Gate", "Silver Fang",
    "Clockwork Ruin", "Sunken Archive", "Grave Protocol", "Warden's Call",
    "Crimson Verdict", "Hollow Crown", "Fractured Sky", "Pale Reckoning",
    "Ember Throne", "Dusk Sentinel", "Chain Breaker", "Mire Walker",
    "Iron Covenant", "Sundered Keep", "Veil of Ash", "Coldfire March",
    "Ruin Herald", "Abyss Warden", "Siege of Dusk", "Broken Signal",
    "The Outer Reach", "Vantage Zero", "Rift Commander", "Dust Protocol",
    "Night Sentinel", "Bone Circuit", "Feral Horizon", "Wrath Engine",
]

TITLES_JP = [
    "影の伝説", "龍の剣", "紅蓮の空", "鉄の拳",
    "凍てつく深淵", "嵐の中の沈黙", "燃える聖戦", "失われた王国",
    "幻影の刃", "鋼の翼", "暗黒の地平線", "ネオンの鎮魂歌",
    "地下室の走者", "砂埃と栄光", "砕けた王座", "エコープロトコル",
    "錆びた王冠", "琥珀の包囲", "水晶の深み", "虚空の番人",
    "灰の道", "虚ろの騎士", "迷走回路", "炎の滝",
    "鉄の潮", "最後の砦", "砕かれた領域", "蒼白の旅人",
    "嵐の略奪者", "バイナリの幽霊", "黒曜石の門", "銀の牙",
    "歯車の廃墟", "沈んだ書庫", "墓の規約", "番人の呼び声",
    "深紅の評決", "虚ろの王冠", "割れた空", "蒼白の断罪",
    "炎の王座", "黄昏の番人", "鎖を断つ者", "沼地の歩者",
    "鉄の盟約", "分断された砦", "灰のヴェール", "冷炎の行進",
    "廃墟の先触れ", "深淵の番人", "黄昏の包囲", "断絶した信号",
    "外なる領域", "零の優位", "亀裂の指揮官", "砂塵の規約",
    "夜の番人", "骨の回路", "荒野の地平", "怒りの機関",
]

TITLES_RU = [
    "Тень Колосса", "Логово дракона", "Пурпурные небеса", "Железный кулак",
    "Ледяная бездна", "Тихая буря", "Пылающий крестовый поход", "Потерянное королевство",
    "Призрачный клинок", "Стальные крылья", "Тёмный горизонт", "Неоновый реквием",
    "Бегуны подземелья", "Пыль и слава", "Сломанный трон", "Эхо-протокол",
    "Ржавая корона", "Янтарная осада", "Хрустальные глубины", "Страж пустоты",
    "Пепельный путь", "Полый рыцарь", "Блуждающий контур", "Угасающий водопад",
    "Железный прилив", "Последний бастион", "Разбитое царство", "Бледный странник",
    "Буревой расхититель", "Двоичный призрак", "Обсидиановые врата", "Серебряный клык",
    "Руины часового механизма", "Затопленный архив", "Протокол могилы", "Зов стража",
    "Пурпурный вердикт", "Полая корона", "Расколотое небо", "Бледное возмездие",
    "Огненный трон", "Страж сумерек", "Разрушитель цепей", "Ходок по трясине",
    "Железный завет", "Разрубленная крепость", "Пелена пепла", "Марш холодного огня",
    "Вестник руин", "Страж бездны", "Осада сумерек", "Разорванный сигнал",
    "Внешний предел", "Нулевое преимущество", "Командир разлома", "Протокол пыли",
    "Ночной страж", "Костяная схема", "Дикий горизонт", "Двигатель гнева",
]

TITLES_ZH = [
    "巨像之影", "龙之巢穴", "赤红苍穹", "铁拳",
    "冰封深渊", "寂静风暴", "燃烧的圣战", "失落王国",
    "幻影之刃", "钢铁之翼", "黑暗地平线", "霓虹镇魂曲",
    "地窖奔跑者", "尘埃与荣耀", "破碎王座", "回声协议",
    "锈蚀王冠", "琥珀围城", "水晶深处", "虚空守卫",
    "灰烬之路", "空洞骑士", "迷途回路", "余烬瀑布",
    "铁潮", "最后堡垒", "破碎领域", "苍白流浪者",
    "风暴掠夺者", "二进制幽灵", "黑曜石之门", "银牙",
    "齿轮废墟", "沉没档案", "墓穴协议", "守卫的呼唤",
    "深红裁决", "空洞王冠", "破裂苍穹", "苍白清算",
    "余烬王座", "黄昏守卫", "断链者", "泥泞行者",
    "铁之契约", "裂开的要塞", "灰烬之幕", "冷焰行军",
    "废墟先驱", "深渊守卫", "黄昏围城", "断裂信号",
    "外部边界", "零号优势", "裂缝指挥官", "尘埃协议",
    "夜间守卫", "骨骼回路", "荒野地平线", "愤怒引擎",
]

LANGUAGES = [
    ("EN", TITLES_EN),
    ("JP", TITLES_JP),
    ("RU", TITLES_RU),
    ("ZH", TITLES_ZH),
]

def pick_game_name():
    lang_code, pool = random.choice(LANGUAGES)
    title = random.choice(pool)
    return lang_code, title

# =========================
# 1. Generate Games (120)
# =========================
games = []
game_meta = []

used_names = set()

for i in range(1, 121):
    uid = i
    game_id = f'ULUS{i:05d}'
    cat = random.choice([0, 1, 2])  # PSP, PS1, Homebrew

    while True:
        lang_code, name = pick_game_name()
        if (lang_code, name) not in used_names:
            used_names.add((lang_code, name))
            break

    name_bytes = name.encode('utf-8')[:63]

    entry = struct.pack(
        '<I16s64s8sB3B',
        uid,
        game_id.encode('ascii'),
        name_bytes,
        b'0x120',
        cat,
        0, 0, 0
    )

    games.append(entry)
    game_meta.append((uid, game_id, lang_code, name, cat))

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

def generate_month_sessions(year, month, days_in_month, count=1000):
    month_start_ts = int(datetime(year, month, 1, tzinfo=timezone.utc).timestamp())
    daily_usage = {d: 0 for d in range(days_in_month)}
    sessions = []
    generated = 0

    while generated < count:
        day      = random.randint(0, days_in_month - 1)
        game_uid = random.randint(1, 120)
        duration = random.randint(300, 2700)

        if daily_usage[day] + duration > 80000:
            continue

        daily_usage[day] += duration
        ts = month_start_ts + (day * 86400) + random.randint(0, 86400 - duration)
        sessions.append(struct.pack('<3I', game_uid, duration, ts))
        generated += 1

    return sessions

sessions = []
sessions += generate_month_sessions(2026, 1, 31)   # January
sessions += generate_month_sessions(2026, 2, 28)   # February
sessions += generate_month_sessions(2026, 3, 31)   # March
sessions += generate_month_sessions(2026, 4, 30)   # April

sessions.sort(key=lambda x: struct.unpack('<3I', x)[2])

with open(os.path.join(TEST_DIR, 'sessions.dat'), 'wb') as f:
    for s in sessions:
        f.write(s)

# =========================
# 3. Debug / Validation
# =========================

month_counts = {1: 0, 2: 0, 3: 0, 4: 0}
MONTH_NAMES  = {1: "January", 2: "February", 3: "March", 4: "April"}

for s in sessions:
    _, _, ts = struct.unpack('<3I', s)
    dt = datetime.utcfromtimestamp(ts)
    if dt.month in month_counts:
        month_counts[dt.month] += 1

CAT_LABELS = {0: "PSP", 1: "PS1", 2: "Homebrew"}

print(f'Generated {len(games)} games and {len(sessions)} sessions.')
print()
for m, label in MONTH_NAMES.items():
    print(f'{label} sessions: {month_counts[m]}')
print()
print(f"{'UID':<6} {'Game ID':<12} {'Lang':<5} {'Category':<10} Title")
print("-" * 90)
for uid, gid, lang, name, cat in game_meta:
    print(f"{uid:<6} {gid:<12} {lang:<5} {CAT_LABELS[cat]:<10} {name}")
