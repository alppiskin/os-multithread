primer = 0
ending = ".txt"

import os

if not os.path.exists("test_files"):
    os.makedirs("test_files")

content = ""
for k in range(1000000):
    content += "random \n"
    
for i in range(400):
    fd = open("test_files/" + str(primer)+ending, "w")
    fd.write(content)
    # print(str(primer))
    primer+=1
