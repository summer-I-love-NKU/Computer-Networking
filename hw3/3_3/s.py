import os,sys

def init_set(arg1=1):#榛樿?ゆ槸1
    global fileID
    fileID=arg1
    with open("input_client",'w') as f:
        f.write(str(fileID))
        
def main():
    arg1 = sys.argv[1]#璇诲彇鍛戒护琛岃緭鍏?
    init_set(arg1)
    os.system("start 1")
    os.system("start 2")
        
main()
        

