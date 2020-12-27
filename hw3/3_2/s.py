import os,sys

def init_set(arg1, arg2=1):#榛樿?ゆ槸1
    global window_width,fileID
    window_width=arg1
    fileID=arg2
    
    with open("input_server",'w') as f:
        f.write(str(window_width))
    with open("input_client",'w') as f:
        f.write(str(window_width)+'\n'+str(fileID))
        
def main():
    arg1, arg2 = sys.argv[1], sys.argv[2]#璇诲彇鍛戒护琛岃緭鍏?
    init_set(arg1, arg2)
    os.system("start 1")
    os.system("start 2")
        
main()
        

