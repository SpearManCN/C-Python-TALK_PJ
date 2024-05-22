import socket
import sys
import threading
import time

# SERVER_IP = 
# SERVER_PORT =     
# BUFFER_SIZE =         

thisUser = ""
nowGroup = ""
running = False
# init ---------------------------------------------------------------------------------------
def printRaws():
    print("")
    print("")
    print("")
    print("")
    print("")
    print("")
    
    
def readChat():
    global running
    global thisUser
    userSay = thisUser+" : "
    while running:
        recv_data = client_socket.recv(BUFFER_SIZE)
        clean_data = recv_data.split('\x00')[0]
        if running == False:
            break
        if "/quit/quit" in clean_data:
            print('\033[32m'+"----------"+clean_data[10:]+" has left -------"+'\033[0m')
        elif "/quit" in clean_data:
            print('\033[32m'+"----------"+clean_data[5:]+" has joined -------"+'\033[0m')
        elif userSay in clean_data:
            width = 50  # The total width of the output
            print('\033[38;5;3m'+clean_data.rjust(width)+'\033[0m')
        elif "--Notice--" in clean_data:
            print('\033[38;5;1m'+clean_data+'\033[0m')
        else:
            print(clean_data)
    
        
        
def login():
    global thisUser
    printRaws()
    print("----------------Login----------------")
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
    printRaws()
    print("-------------Create group------------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
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
    printRaws()
    print("-----------Withdraw a group----------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
    print("----Users Number------Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "               " + groups[0] )
    print("")
    print("Please enter the group name you wish to withdraw from.")
    print("Back to the Lobby = 'quit'")
    groupName = raw_input()
    if groupName == 'quit':
        lobby()
    sendM = "72"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
    client_socket.sendall(sendM.encode())
    recvData = client_socket.recv(BUFFER_SIZE)
    clean_data = recvData.split('\x00')[0]
    if clean_data == '1':
        print("Do you want to withdraw the group ("+groupName +")? y/any other key")
        yOrN = raw_input()
        if yOrN == 'y' or yOrN == 'Y':
            sendM = "73"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
            client_socket.sendall(sendM.encode())
            lobby()
        else:
            withdrawGroup(originData)
    else:
        print("You cannot withdraw that group")
        withdrawGroup(originData)
    
def lobby():
    global thisUser
    printRaws()
    
    print("----------------Lobby----------------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
    print("Choose what you want to do")
    print("Enter '1' = Enter a group you have joined")
    print("Enter '2' = Join a group")
    print("Enter '3' = Create a group")
    print("Enter '4' = Withdraw a group")
    print("Enter '5' = Remove a group")
    print("Enter others = Back to the home")
    act = raw_input()
    if act == '1': # Enter
        # print("act1")
        sendM = "31"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        groupEnterList(data)
    elif act == '2': # join
        # print("act2")
        sendM = "32"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        groupJoinList(data)
    elif act == '3': # creategroup
        # print("act3")
        createGroup()
    elif act == '4': # withdraw
        # print("act4")
        sendM = "71"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        withdrawGroup(data)
    elif act == '5': # remove
        # print("act5")
        sendM = "51"+thisUser+"\0"
        client_socket.sendall(sendM.encode())
        data = client_socket.recv(BUFFER_SIZE)
        removeGroup(data)
    else :
        # print("else ")
        thisUser = ""
        welcome()

def getPreChat():
    global thisUser
    global nowGroup
    sendM = "83"+str(len(thisUser))+thisUser+"\0"+nowGroup+"\0"
    client_socket.sendall(sendM.encode())

def enterGroup(groupName):
    global thisUser
    global nowGroup
    global running
    nowGroup = groupName
    printRaws()    
    print("-----------Chatting group ["+"\033[38;5;2m"+groupName+"\033[0m"+"]-----------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
    print("---------------User List-------------")
    sendM = "82"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
    client_socket.sendall(sendM.encode())
    data = client_socket.recv(BUFFER_SIZE)
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        print ( "   "+datas )
    running = True
    thread = threading.Thread(target=readChat)
    thread.start()
    getPreChat()
    time.sleep(1)
    writeChat(1)

def removeGroup(data):
    global thisUser
    yOrN = ""
    originData = data
    printRaws()    
    print("-----------Remove a group------------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
    print("---joined users-----now Users-----Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "              " +groups[2] +"             "+ groups[0] )
    print("")
    print("Deletion is possible only when there are 0 users currently in the group.")
    print("Please select the name of the room and enter")
    print("Back to the Lobby = 'quit'")
    groupName = raw_input()
    if groupName == 'quit':
        lobby()
    else:
        sendM = "52"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
        client_socket.sendall(sendM.encode())
    recvData = client_socket.recv(BUFFER_SIZE)
    clean_data = recvData.split('\x00')[0]
    if clean_data == "0":
        print("You cannot remove that group")
        removeGroup( originData )
    else:
        print("Do you want to remove the group ("+groupName +")? y/any other key")
        yOrN = raw_input()
    if yOrN == 'y' or yOrN == 'Y':
        sendM = "53"+str(len(thisUser))+thisUser+"\0"+groupName+"\0"
        client_socket.sendall(sendM.encode())
        lobby()
    else :
        removeGroup( originData )
    

def groupEnterList( data ):
    global thisUser
    yOrN = ""
    originData = data
    printRaws()
    print("-----------Enter a group-----------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
    print("---Users Number-----Group Name-----")
    clean_data = data.split('\x00')[0].split(',')
    for datas in clean_data[:-1]:
        groups = datas.split(':')
        print ( "        "+groups[1] + "               " + groups[0] )
    
    printRaws()
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
        printRaws()
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
    printRaws()
    print("-----------Join a group-----------["+"\033[38;5;2m"+thisUser+"\033[0m"+"]")
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
    printRaws()
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
      
def exitProgram():
    client_socket.close()
    sys.exit(0)
    
def welcome():
    printRaws()
    print("-----------Welcome-----------")
    print("If you have an ID = press 1, and if you want to sign up press any other.")
    print("exit = '/quit' ")
    loginOrSign = raw_input()
    # print(loginOrSign)
    if loginOrSign == "1":
        # print("login")
        login()
    elif loginOrSign == "/quit":
        exitProgram()
    else :
        # print("signup")
        signUp()
    
def leaveGroup():
    global thisUser
    global nowGroup
    global running
    sendM2 = "03"+str(len(thisUser))+thisUser+"\0"+nowGroup+"\0"
    client_socket.sendall(sendM2.encode())
    
    sendM = "9"+str(len(thisUser))+thisUser+"\0"+nowGroup+"\0"
    client_socket.sendall(sendM.encode())
    
    nowGroup = ""
    

    
def writeChat(isFirst):
    
    global thisUser
    global nowGroup
    global running
    lengthG = len(nowGroup) + 10
    if isFirst == 1:
        sendM2 = "01"+str(len(thisUser))+thisUser+"\0"+nowGroup
        client_socket.sendall(sendM2.encode())
    while True:
        sendM = raw_input()
        if sendM == "/quit":
            print("Are you sure you want to go out? y/any other key")
            yOrN = raw_input()
            if yOrN == 'y' or yOrN == 'Y':
                running = False
                leaveGroup()
                lobby()
                break
            else :
                writeChat(0)
                break
        sendM2 = "02"+str(len(thisUser))+thisUser+"\0"+str(lengthG)+"\0"+nowGroup+"\0"+sendM+"\0"
        client_socket.sendall(sendM2.encode())


# init ---------------------------------------------------------------------------------------

# main ---------------------------------------------------------------------------------------
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
