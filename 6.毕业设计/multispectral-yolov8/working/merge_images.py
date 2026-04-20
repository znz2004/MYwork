import cv2
import numpy as np
from pathlib import Path

# ========== 设置路径 ==========
# 你的检测结果文件夹
detect_dir = Path("runs/detect/exp")  # 修改为你的实际 exp 文件夹

# 输出文件夹
output_dir = Path("runs/detect/exp/combined")
output_dir.mkdir(parents=True, exist_ok=True)

# ========== 查找所有 rgb 和 ir 图片 ==========
rgb_files = list(detect_dir.glob("*_rgb.jpg"))
ir_files = list(detect_dir.glob("*_ir.jpg"))

print(f"find {len(rgb_files)} pictures")
print(f"find {len(ir_files)} pictures")

# ========== 合并图片 ==========
merged_count = 0

for rgb_path in rgb_files:
    # 获取基础名称（去掉 _rgb.jpg）
    base_name = rgb_path.stem.replace("_rgb", "")
    
    # 对应的 IR 图片路径
    ir_path = detect_dir / f"{base_name}_ir.jpg"
    
    if ir_path.exists():
        # 读取图片
        img_rgb = cv2.imread(str(rgb_path))
        img_ir = cv2.imread(str(ir_path))
        
        # 确保两张图片尺寸相同（如果不同，调整 IR 到 RGB 尺寸）
        if img_rgb.shape != img_ir.shape:
            img_ir = cv2.resize(img_ir, (img_rgb.shape[1], img_rgb.shape[0]))
        
        # 左右合并
        combined = np.hstack((img_rgb, img_ir))
        
        # 添加文字标注
        cv2.putText(combined, "RGB", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(combined, "IR", (img_rgb.shape[1] + 10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)
        
        # 保存合并后的图片
        save_path = output_dir / f"{base_name}_merged.jpg"
        cv2.imwrite(str(save_path), combined)
        merged_count += 1

print(f"finish!")