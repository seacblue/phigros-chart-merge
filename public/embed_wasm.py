import base64
import argparse

def embed_wasm_in_html(wasm_path, html_path, output_path=None):
    """
    将 WASM 文件转换为 base64 并嵌入到 HTML 中的占位符位置
    """
    # 读取 WASM 文件并转换为 base64
    with open(wasm_path, 'rb') as f:
        wasm_bytes = f.read()
    wasm_base64 = base64.b64encode(wasm_bytes).decode('utf-8')
    
    # 读取 HTML 文件
    with open(html_path, 'r', encoding='utf-8') as f:
        html_content = f.read()
    
    # 替换占位符
    modified_html = html_content.replace(
        '<!-- WASM_BASE64_PLACEHOLDER -->',
        wasm_base64
    )
    
    # 输出结果
    output = output_path or html_path
    with open(output, 'w', encoding='utf-8') as f:
        f.write(modified_html)
    
    print(f"成功将 WASM 嵌入到 {output}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='将 WASM 文件转换为 base64 并嵌入 HTML')
    parser.add_argument('wasm_file', help='WASM 文件路径')
    parser.add_argument('html_file', help='HTML 文件路径')
    parser.add_argument('-o', '--output', help='输出 HTML 文件路径（默认覆盖原文件）')
    args = parser.parse_args()
    
    embed_wasm_in_html(args.wasm_file, args.html_file, args.output) 