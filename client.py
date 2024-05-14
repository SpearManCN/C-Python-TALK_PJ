import socket
import threading
import time

SERVER_IP = '' 
SERVER_PORT = 1056        
BUFFER_SIZE = 1024        

thisUser = ""
nowGroup = ""
# init -----------------------------------
def chatStart():
    while True:
        message = raw_input()
        if message == 'quit':
            close_data = message
            break
    client_socket.send(message.encode())

def login():
    global thisUser
    print("-----------Login-----------")
    print("Enter your ID")
    id = raw_input()
    print("Enter your PW")
    pw = raw_input()
    sendM = "2"+str(len(id))+str(len(pw))+id+"\0"+pw+"\0"
    client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0]
    if clean_data == '0':
        print("The ID or PW is incorrect.")
        login()
    else :
        thisUser = id
        lobby()
    
def createGroup():
    global thisUser
    isTrue = False
    groupName = ""
    print("---------Create group---------["+thisUser+"]")
    while not isTrue:
        print("Please enter the new group name (1 ~ 30 letters)")
        groupName = raw_input()
        if groupName == "quit":
            lobby()
        elif len(groupName)>0 and len(groupName)<31 :
            isTrue = True
        else:
            print("Invalid format")
    sendM = "4"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
    client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0]
    if clean_data == '1' :
        print("created group")
        lobby()
    else:
        print("This name already exists")
        createGroup()
    
def withdrawGroup(data):
    global thisUser
    yOrN = ""
    originData = data
    print("-----------Withdraw a group-----------["+thisUser+"]")
    print("---Users Number-----Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "               " + groups[0] )
    


    
def lobby():
    global thisUser
    print("-----------Lobby-----------["+thisUser+"]")
    print("Choose what you want to do")
    print("Enter '1' = Enter a group you have joined")
    print("Enter '2' = Join a group")
    print("Enter '3' = Create a group")
    print("Enter '4' = Withdraw a group")
    print("Enter others = Back to the home")
    act = raw_input()
    if act == '1':
        print("act1")
        sendM = "31"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        groupEnterList(data)
    elif act == '2':
        print("act2")
        sendM = "32"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        groupJoinList(data)
    elif act == '3':
        print("act3")
        createGroup()
        # creategroup
    elif act == '4':
        print("act4")
        sendM = "71"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        withdrawGroup(data)
    else :
        print("else ")
        thisUser = ""
        welcome()

def enterGroup(groupName):
    global thisUser
    print("-----------Chatting group ["+groupName+"]-----------["+thisUser+"]")
    print("---------------User List--------------")
    sendM = "82"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
    client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        print ( "   "+datas )

def groupEnterList( data ):
    global thisUser
    yOrN = ""
    originData = data
    print("-----------Enter a group-----------["+thisUser+"]")
    print("---Users Number-----Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "               " + groups[0] )
    
    print("")
    print("Please select the name of the room and enter")
    print("Back to the Lobby = 'quit'")
    groupName = raw_input()
    if groupName == "quit":
        lobby()
    else:
        sendM = "81"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
        client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0]
    if clean_data == "0":
        print("You cannot enter that group")
        groupEnterList( originData )
    else:
        print("Do you want to enter the group ("+groupName +")? y/any other key")
        yOrN = raw_input()
    if yOrN == 'y' or yOrN == 'Y':
        enterGroup( groupName )
    else :
        groupEnterList( originData )
        
        
    # here 

def groupJoinList( data ):
    global thisUser
    yOrN = ""
    originData = data
    print("-----------Join a group-----------["+thisUser+"]")
    print("---Users Number-----Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "               " + groups[0] )
    
    print("")
    print("Please select the name of the room and enter")
    print("Back to the Lobby = 'quit'")
    groupName = raw_input()
    if groupName == "quit":
        lobby()
    else:
        sendM = "6"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
        client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0]
    if clean_data == "0":
        print("You cannot Join that group")
        groupJoinList( originData )
    else:
        print("You have joined the group"+groupName)
        lobby()
        
def signUp():
    global thisUser
    idTrue = False
    pwTrue = False
    id = ""
    pw = ""
    print("-----------Sign Up-----------")
    while not idTrue :
        print("Enter the ID you want to create (1 ~ 9 letters)")
        id = raw_input()
        if len(id)>0 and len(id)<10:
            idTrue = True
        else:
            print("Invalid format.")
        
    while not pwTrue:
        print("Enter the password (1 ~ 9 letters)")
        pw = raw_input()
        print("Please re-enter your password to confirm")
        pw2 = raw_input()
        if pw == pw2 and len(pw)<10:
            pwTrue = True
        else:
            print("Please check your password again")

    sendM = "1"+str(len(id))+str(len(pw))+id+"\0"+pw+"\0"
    client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0]
    if clean_data == "0":
        print("This id already exists.")
        signUp()
    else :
        thisUser = id
        lobby()
    
    
def welcome():
    print("-----------Welcome-----------")
    print("If you have an ID, press 1, and if you want to sign up, press any other.")
    loginOrSign = raw_input()
    print(loginOrSign)
    if loginOrSign == "1":
        print("login")
        login()
    else :
        print("signup")
        signUp()
    
    
def recv_data(client_socket):
    while True:
        data = client_socket.recv(1024)
        clean_data = data.decode().split('\x00')[0]
        print("recive : ", clean_data[1:])

# init -----------------------------------


print ( "started")
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
connected = False
while not connected:
    try:
        client_socket.connect((SERVER_IP, SERVER_PORT))
        print("success to connet")
        connected = True
        welcome()
    except socket.error:
        print("failed to connect. reconnecting...")
        
        time.sleep(1)  


# client_socket.connect((SERVER_IP, SERVER_PORT))
# print("success to connet")
# try:


# my_thread = threading.Thread(target=recv_data, args=(client_socket,))
# my_thread.start()

client_socket.close()


# while True:
#     message = input("ff")
#     client_socket.sendall(message.encode())
    
#     # 
#     data = client_socket.recv(BUFFER_SIZE)
#     print('read message : ', data.decode())

# # except:
#     print("ff")

# # finally:
#     client_socket.close()