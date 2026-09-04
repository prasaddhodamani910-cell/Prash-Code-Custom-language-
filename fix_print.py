import re

with open('src/codegen.cpp', 'r') as f:
    content = f.read()

# Fix _print_int manually
start_idx = content.find('emitLabel("_print_int");')
end_idx = content.find('// write syscall', start_idx)
end_idx = content.find('ret");', end_idx) + 6

print_int_orig = content[start_idx:end_idx]

# Remove the callee saved push/pop from print_int_orig
new_print_int = re.sub(r'    for \(int i = 19; i <= 27; i \+= 2\) \{\n        emit\("stp x" \+ std::to_string\(i\) \+ ", x" \+ std::to_string\(i\+1\) \+ ", \[sp, #-16\]!"\);\n    \}\n', '', print_int_orig)
new_print_int = re.sub(r'    for \(int i = 27; i >= 19; i -= 2\) \{\n        emit\("ldp x" \+ std::to_string\(i\) \+ ", x" \+ std::to_string\(i\+1\) \+ ", \[sp\], #16"\);\n    \}\n', '', new_print_int)

content = content[:start_idx] + new_print_int + content[end_idx:]

with open('src/codegen.cpp', 'w') as f:
    f.write(content)
