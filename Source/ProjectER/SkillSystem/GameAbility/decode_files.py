import os

def decode_file(src_path, dst_path):
    try:
        with open(src_path, 'rb') as f:
            data = f.read()
        
        # Try cp949 decoding
        decoded = data.decode('cp949', errors='replace')
        
        with open(dst_path, 'w', encoding='utf-8') as f:
            f.write(decoded)
        print(f"Decoded {src_path} to {dst_path}")
    except Exception as e:
        print(f"Error decoding {src_path}: {e}")

if __name__ == "__main__":
    files = [
        (r'd:\unreal\Project_JSER\Source\ProjectER\SkillSystem\GameAbility\SkillBase.cpp', 'skill_base_orig.txt'),
        (r'd:\unreal\Project_JSER\Source\ProjectER\SkillSystem\GameplayEffectComponent\SummonRangeBaseGEC.cpp', 'summon_range_orig.txt'),
        (r'd:\unreal\Project_JSER\Source\ProjectER\SkillSystem\GameplayEffectComponent\LaunchHomingMissile.cpp', 'launch_homing_orig.txt')
    ]
    
    out_dir = r'C:\Users\KIMHABIN\.gemini\antigravity\brain\89940104-689c-4ac2-8091-e9ee3f4584a0'
    for src, dst in files:
        decode_file(src, os.path.join(out_dir, dst))
