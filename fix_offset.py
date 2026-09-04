import re

with open('src/codegen.cpp', 'r') as f:
    content = f.read()

# Change m_currentFpOffset = 0 to m_currentFpOffset = -80 in generateFnDecl
content = content.replace('m_currentFpOffset = 0;', 'm_currentFpOffset = -80;')

with open('src/codegen.cpp', 'w') as f:
    f.write(content)
