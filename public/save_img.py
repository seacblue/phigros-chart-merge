import base64
import os

def img_to_base64(img_path, output_txt_path=None):
    # 获取图像格式（如 png、jpg）
    img_ext = os.path.splitext(img_path)[1].strip('.').lower()
    if img_ext not in ['png', 'jpg', 'jpeg', 'gif', 'svg']:
        print(f"不支持的图像格式：{img_ext}")
        return

    # 读取图像文件并转为 base64
    with open(img_path, 'rb') as f:
        img_bytes = f.read()
    img_base64 = base64.b64encode(img_bytes).decode('utf-8')

    # 生成 HTML 可用的 data URL
    data_url = f'data:image/{img_ext};base64,{img_base64}'

    # 保存到文本文件
    if output_txt_path:
        with open(output_txt_path, 'w', encoding='utf-8') as f:
            f.write(data_url)
        print(f"base64 已保存到：{output_txt_path}")
    else:
        print("图像 base64 Data URL（直接复制到 HTML）：")
        print(data_url)
    return data_url

if __name__ == "__main__":
    # 替换为你的图像文件路径
    img_path = "icons/merge.png"
    # 输出的文本文件路径
    output_txt_path = "img_base64.txt"
    img_to_base64(img_path, output_txt_path)