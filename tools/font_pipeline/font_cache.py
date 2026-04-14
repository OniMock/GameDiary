import os
import json
import hashlib

def get_hash(text):
    return hashlib.sha256(text.encode('utf-8')).hexdigest()

class FontCache:
    def __init__(self, cache_file):
        self.cache_file = cache_file
        self.cache = {}
        if os.path.exists(cache_file):
            try:
                with open(cache_file, 'r') as f:
                    self.cache = json.load(f)
            except Exception:
                pass

    def has_changed(self, group_name, charset_text):
        new_hash = get_hash(charset_text)
        old_hash = self.cache.get(group_name)
        return new_hash != old_hash

    def update(self, group_name, charset_text):
        self.cache[group_name] = get_hash(charset_text)
        
    def save(self):
        with open(self.cache_file, 'w') as f:
            json.dump(self.cache, f)

