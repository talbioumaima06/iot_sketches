def file_to_c_array(filename):
    with open(filename, 'rb') as f:
        byte_arr = f.read()
        c_array = ', '.join([f'0x{byte:02x}' for byte in byte_arr])
        return c_array, len(byte_arr)

c_array_content, array_length = file_to_c_array('simple_index.zip')

print(f'const uint8_t index_ov2640_html_gz[] = {{ {c_array_content} }};')
print(f'#define index_ov2640_html_gz_len {array_length};')
