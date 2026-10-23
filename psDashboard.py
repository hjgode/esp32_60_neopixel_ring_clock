# Designed with https://visualtkinter.com - paste this file into File > Import there to edit it visually
import tkinter as tk
from tkinter import ttk
from tkinter import colorchooser
from paho.mqtt import client as mqtt_client
import json

broker = '192.168.0.40' #'iot.reyax.com'
port = 1883
topic_watch ="/esp32_clock/#"
topic_pub = "/esp32_clock/dumpValues"
topic_sub = "/esp32_clock/settings"
# generate client ID with pub prefix randomly
client_id = 'esp32_dashboard'
username = ''
password = ''
deviceId = "esp32_dashboard"

#global dictionary for the values
mySettingsDict={}
mySettingsDict["colorHour"] = 0x1f
mySettingsDict["colorMinute"] = 0x1f
mySettingsDict["colorSecond"] = 0x1f
mySettingsDict["brightness"] = 20
mySettingsDict["tickColor"] = 0x00323232
mySettingsDict["dimOn"] = True
mySettingsDict["valueDim"] = 10
mySettingsDict["hourStartDim"] = 20
mySettingsDict["hourStopDim"] = 6
#mySettingsDict["dumpValues
#mySettingsDict["save_Pref
#load_Pref
#mySettingsDict["getAutoDimmed

mySettingsDict["isAutoDimmed"] = True

myTopicMappingDict={}
myTopicMappingDict["colorHour"] = "handleHourColor"
myTopicMappingDict["colorMinute"] = "handleMinuteColor"
myTopicMappingDict["colorSecond"] = "handleSecondColor"
myTopicMappingDict["brightness"] = "brightness"
myTopicMappingDict["tickColor"] = "tickColor"
myTopicMappingDict["dimOn"] = "dimOn"
myTopicMappingDict["valueDim"] = "valueDim"
myTopicMappingDict["hourStartDim"] = "hourStartDim"
myTopicMappingDict["hourStopDim"] = "hourStopDim"

def getTopicForKey(what:str):
    if(what in myTopicMappingDict):
        return(myTopicMappingDict[what])
    pass

'''
  possible mqtt values
  brightness  0-255
  offset      0-59
  handleMinuteColor as 32Bit Hex number, ie 001f3a40
  handleHourColor
  handleSecondColor
  tickColor
  dimOn
  valueDim
  hourStartDim
  hourStopDim
  dumpValues
  save_Pref
  load_Pref
  getAutoDimmed

  doc["offset"]=OFFSET;
  doc["brightness"]=PIXEL_BRIGHTNESS;
  doc["handleHourColor"]=handleHourColor;
  doc["handleMinuteColor"]=handleMinuteColor;
  doc["handleSecondColor"]=handleSecondColor;
  doc["tickColor"]=tickColor;
  doc["dimOn"]=dimOn;
  doc["valueDim"]=valueDim;
  doc["hourStartDim"]=hourStartDim;
  doc["hourStopDim"]=hourStopDim;
  getAutoDimmed

  out only:
  settings
  isAutoDimmed
'''

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc==0:
            print("Successfully connected to MQTT broker")
        else:
            print("Failed to connect, return code %d", rc)
#    client = mqtt_client.Client(client_id)
    client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION1, client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def publish(client,status):
    # msg = f"messages: {msg_count}"
    msg="dumpValues";
    #msg = "{\"action\":\"command/insert\",\"deviceId\":\""+deviceId+"\",\"command\":{\"command\":\"LED_control\",\"parameters\":{\"led\":\""+status+"\"}}}"
    # msg = '{"action":"command/insert","command":{"id":432436060,"command":"LED_control","timestamp":"2021-03-24T00:19:44.418","lastUpdated":"2021-03-24T00:19:44.418","userId":37,"deviceId":"s3s9TFhT9WbDsA0CxlWeAKuZykjcmO6PoxK6","networkId":37,"deviceTypeId":5,"parameters":{"led":"on"},"lifetime":null,"status":null,"result":null},"subscriptionId":1616544981034531}'
    result = client.publish(topic, msg)
    # result: [0, 1]
    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{topic}`")
    else:
        print(f"Failed to send message to topic {topic}")
def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        #print(f"Recieved '{msg.payload.decode()}' from '{msg.topic}' topic")
        msgTemp=msg.payload.decode();
        if(type(msgTemp) is str and msgTemp.endswith["\0"]):
            msgTemp=msgTemp[:-1]
        print(f"msg: {msgTemp}")
        y = json.loads(msgTemp)# msg.payload.decode())
        
        hourStartDim=str(y["hourStartDim"])
        hourStopDim=str(y["hourStopDim"])
        
#        temp = str(y["settings"]) #["parameters"]["temp"])
#        hum = 0 #str(y["notification"]["parameters"]["humi"])
#        print("temperature: ",temp,", humidity:",hum)
        print("hourStartDim: ", hourStartDim, "hourStopDim: ", hourStopDim)
        
        
        temp_label.config(text=hourStopDim,
                        fg="black")
        hum_label.config(text=hourStartDim,
                          fg="black")
    client.subscribe(topic_sub)
    client.on_message = on_message

'''
  doc["offset"]=OFFSET;
  doc["brightness"]=PIXEL_BRIGHTNESS;
  doc["handleHourColor"]=handleHourColor;
  doc["handleMinuteColor"]=handleMinuteColor;
  doc["handleSecondColor"]=handleSecondColor;
  doc["tickColor"]=tickColor;
  doc["dimOn"]=dimOn;
  doc["valueDim"]=valueDim;
  doc["hourStartDim"]=hourStartDim;
  doc["hourStopDim"]=hourStopDim;
'''
# print(d["name"])     # Access using key
# print(d.get("age"))  # Access using get()

class MQTTClient:
    def __init__(self, broker, port, wind):
        self.client = mqtt_client.Client()
        self.broker = broker
        self.port = port

        self.callbackWindow=wind

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message


    def connect(self):
        self.client.connect(self.broker, self.port, 60)
        self.client.loop_start()

    def on_connect(self, client, userdata, flags, rc):
        print(f"Connected with result code {rc}")
        self.client.subscribe(topic_watch)
        #self.client.subscribe(topic_watch)
        
    def on_message(self, client, userdata, msg):
        print(f"Message received: {msg.topic} {msg.payload}")
        payload=msg.payload.decode()
        if(type(payload) is str):
            if(payload.endswith("\0")):
                payload=payload[:-1]
        if(msg.topic.endswith("settings")):
            if (not payload.endswith("}")):
                payload=payload + "}"
            self.callbackWindow.updateDumpTxt(payload)
        if(msg.topic.endswith("isAutoDimmed")):
            self.callbackWindow.updateAutoDimmed(payload)
            pass

    def publish(self, topic, message):
        result = self.client.publish(topic, message)
        return result

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        
class Main:
    def __init__(self, root):
        self.root=root
        self.mqtt_client = MQTTClient(broker, port, self)
        self.mqtt_client.connect()
        #query settings
        msg=1
        result = self.mqtt_client.publish(topic_pub, "1") #request settings
        # result: [0, 1]
        status = result[0]
        if status == 0:
                print(f"Send `{msg}` to topic `{topic_pub}`")
        else:
                print(f"Failed to send message to topic {topic_pub}")

                result = self.mqtt_client.publish(topic_pub, "1") #request settings

        self.root = root
        root.title("Main")
        root.geometry("600x480")
        root.resizable(False, False)
        
        global mySettingsDict

        self.settings = None
        self.SettingsValue = None
        self.UpdateForm=None
        self.helligkeitsForm=None

        self.colorHour=0x1f
        self.colorMinute=0x1f
        self.colorSecond=0x1f
        self.brightness=20
        self.tickColor=0x00323232
        self.dimOn=True
        self.valueDim=10
        self.hourStartDim=20
        self.hourStopDim=6
        #self.dumpValues
        #self.save_Pref
        #load_Pref
        #self.getAutoDimmed
        self.isAutoDimmed=True

        self.buttonDumpValues = tk.Button(root, text="Get Settings", command=self.dumpSettings)
        self.buttonDumpValues.place(x=20, y=20, width=96, height=32)

        self.jsonSettings = tk.Text(root)
        self.jsonSettings.place(x=130, y=20, width=220, height=110)

        self.colorHour=0x1f
        self.colorMinute=0x1f
        self.colorSecond=0x1f

        self.label1 = tk.Label(root, text="Zeiger Farben:")
        self.label1.place(x=30, y=140, width=140, height=30)

        self.label2 = tk.Label(root, text="Stunden;")
        self.label2.place(x=40, y=177, width=90, height=26)

        self.textHandleHourColor = tk.Text(root)
        self.textHandleHourColor.place(x=150, y=180, width=80, height=20)

        self.buttonColorHour = tk.Button(root, text="Farbe", command=self.getColorHour)
        self.buttonColorHour.place(x=230, y=174, width=96, height=32)

        self.buttonColorMinute = tk.Button(root, text="Farbe", command=self.getColorMinute)
        self.buttonColorMinute.place(x=230, y=220, width=96, height=32)

        self.textHandleMinuteColor = tk.Text(root)
        self.textHandleMinuteColor.place(x=150, y=226, width=80, height=20)

        self.label3 = tk.Label(root, text="Minuten;")
        self.label3.place(x=40, y=223, width=90, height=26)

        self.label4 = tk.Label(root, text="Sekunden")
        self.label4.place(x=40, y=270, width=90, height=26)

        self.textHandleSecondColor = tk.Text(root)
        self.textHandleSecondColor.place(x=150, y=276, width=80, height=20)

        self.buttonColorSecond = tk.Button(root, text="Farbe", command=self.getColorSecond)
        self.buttonColorSecond.place(x=230, y=270, width=96, height=32)

        # Helligkeit
        self.label5 = tk.Label(root, text="Helligkeit:")
        self.label5.place(x=55, y=320, width=90, height=26)

        self.textBrightness = tk.Text(root)
        self.textBrightness.place(x=150, y=330, width=60, height=20)

        self.buttonHelligkeit = tk.Button(root, text="Helligkeit", command=self.on_buttonHelligkeit)
        self.buttonHelligkeit.place(x=230, y=324, width=96, height=32)

        # tickColor
        self.TickColor = tk.Label(root, text="TickColor:")
        self.TickColor.place(x=55, y=370, width=90, height=26)

        self.textTickColor = tk.Text(root)
        self.textTickColor.place(x=150, y=376, width=60, height=20)

        self.buttonTickColor = tk.Button(root, text="Farbe", command=self.on_buttonTickColor)
        self.buttonTickColor.place(x=230, y=370, width=96, height=32)

        # autoDim
        self.checkbuttonAutoDim_var=tk.BooleanVar(value=self.dimOn)
        self.checkbuttonAutoDim = tk.Checkbutton(root, text="AutoDim", variable=self.checkbuttonAutoDim_var, onvalue=True, offvalue=False, command=self.on_DimOnChange)
        self.checkbuttonAutoDim.place(x=380, y=180, width=120, height=26)

        self.label10 = tk.Label(root, text="Start:")
        self.label10.place(x=380, y=210, width=70, height=30)

        self.spinAutoDimStartValue=tk.StringVar(value=int(self.hourStartDim))
        self.spinboxAutoDimStart = tk.Spinbox(root, from_=0, to=23, textvariable=self.spinAutoDimStartValue, command=self.on_AutoDimStart_changed)
        self.spinboxAutoDimStart.place(x=460, y=210, width=70, height=30)
        self.spinboxAutoDimStart.setvar

        self.label11 = tk.Label(root, text="Stop:")
        self.label11.place(x=380, y=246, width=70, height=30)

        self.spinAutoDimStopValue=tk.StringVar(value=str(self.hourStopDim))
        self.spinboxAutoDimStop = tk.Spinbox(root, from_=0, to=23, textvariable=self.spinAutoDimStopValue,command=self.on_AutoDimStop_changed)
        self.spinboxAutoDimStop.place(x=460, y=245, width=70, height=30)

        # valueDim
        self.buttonValueDim = tk.Button(root, text="Helligkeit", command=self.onValueDimClicked)
        self.buttonValueDim.place(x=465, y=286, width=96, height=32)

        self.textValueDim = tk.Text(root)
        self.textValueDim.place(x=395, y=286, width=60, height=20)
        self.setText(self.textValueDim, self.valueDim)

        #update
        self.buttonUpdate = tk.Button(root, text="Send Update", command=self.on_sendUpdate_clicked)
        self.buttonUpdate.place(x=482, y=20, width=96, height=32)

        #prefs
        self.buttonLoadPrefs = tk.Button(root, text="Load Prefs", command=self.sendLoadPrefs)
        self.buttonLoadPrefs.place(x=482, y=69, width=96, height=32)

        self.buttonSavePrefs = tk.Button(root, text="Save Prefs", command=self.sendSavePrefs)
        self.buttonSavePrefs.place(x=482, y=108, width=96, height=32)

        self.button1Close = tk.Button(root, text="Close", command=self.onButtonClose_Click)
        self.button1Close.place(x=482, y=386, width=96, height=32)

        # AutoDim status
        self.autodim_status=tk.StringVar() #using StringVar binde changes to UI without code
        self.autodim_status.set("autodim status")
        self.textAutonDimOnOff = tk.Label(root, textvariable=self.autodim_status)
        self.textAutonDimOnOff.place(x=400, y=340, width=170, height=20)

        pass

    def onButtonClose_Click(self):
        root.destroy()
        pass
    
    def on_DimOnChange(self):
        self.dimOn=self.checkbuttonAutoDim_var.get()
        print ("DimOn changed: " + str(self.dimOn))
        pass

    def on_AutoDimStart_changed(self):
        self.hourStartDim=self.spinAutoDimStartValue.get()
        # print("AutoDimStart changed: " + str(self.hourStartDim))
        pass
    def on_AutoDimStop_changed(self):
        self.hourStopDim=self.spinAutoDimStopValue.get()
        pass
    
    def setHelligkeit(self, value):
        self.brightness = value;
        self.setText(self.textBrightness, value)
        pass

    def setTickColor(self, value):
        self.tickColor=value
        self.setText(self.textTickColor, value)
        pass

    def setText(self, txtObj, txt):
        txtObj.delete(1.0, "end")
        txtObj.insert(1.0, txt)

    def updateAutoDimmed(self, txt):
        self.autodim_status.set("AutoDim: " + txt)
        pass

    def updateDumpTxt(self, txt):
        if(not txt.endswith("}")):
            txt=txt.join("}")
        self.jsonSettings.delete(1.0, "end")
        self.jsonSettings.insert(1.0, txt)

        msgTemp=self.jsonSettings.get("1.0","end")
        y = json.loads(msgTemp)# msg.payload.decode())
        # ie 16711680
        '''
            b = (rgbw & 0x000000FF);
            g = (rgbw & 0x0000FF00) >> 8;
            r = (rgbw & 0x00FF0000) >> 16;
            w = (rgbw & 0xFF000000) >> 24;
        '''
        rgbw=y["handleHourColor"]
        xR=(rgbw & 0x00ff0000)
        xG=(rgbw & 0x0000ff00)
        xB=(rgbw & 0x000000ff)
        
        self.colorHour=y["handleHourColor"]
        self.colorMinute=y["handleHourColor"]
        self.colorSecond=y["handleSecondColor"]

        self.brightness=y["brightness"]
        self.tickColor=y["tickColor"]
        self.dimOn=y["dimOn"]
        self.valueDim=y["valueDim"]
        self.hourStartDim=y["hourStartDim"]
        self.hourStopDim=y["hourStopDim"]
        #self.isAutoDimmed=y["isAutoDimmed"]        

        # Handle Colors
        self.setText(self.textHandleHourColor, str(y["handleHourColor"]))
        self.setText(self.textHandleMinuteColor, str(y["handleMinuteColor"]))
        self.setText(self.textHandleSecondColor, str(y["handleSecondColor"]))

        # Helligkeit
        self.setText(self.textBrightness, self.brightness)
        self.setTickColor(self.tickColor)

        #update mySettingsDict
        cnt=0
        for key in y:
            data=y[key]
            mySettingsDict[key]=data
            cnt=cnt+1
        print("updated mySettingsDict: " + str(cnt))

        # query auto dim status
        result = self.mqtt_client.publish("/esp32_clock/getAutoDimmed", "1") #request autodimmed
        status = result[0]
        if status == 0:
                print(f"Send `getAutoDim` to topic `/esp32_clock/getAutoDimmed`")
        else:
                print(f"Failed to send message to topic /esp32_clock/getAutoDimmed")

        pass

    def update(self, n):
        self.SettingsValue = n
        print(self.SettingsValue)
  
    def updateLabelinSettings(self):
        #prompt = askinteger("Input", "Input an Integer")
        if self.settings != None:
            self.settings.label.configure(text="Updated")

    def dumpSettings(self):
        pass

    def getText(self, txtbox):
        txt=txtbox.get("1.0", "end")
        if (txt.endswith("\n")):
            txt=txt[:-1]
        return txt
    
    # use right publish topics not yet in dict!
    # see getTopicForKey(what)
    #TODO dumpValues and topics to be used do not have the same names!
    def updateDictSettings(self):
        for key in mySettingsDict.keys():
            if(key=="colorSecond"):
                mySettingsDict[key]=self.getText(self.textHandleSecondColor)
            elif(key=="colorHour"):
                mySettingsDict[key]=self.getText(self.textHandleHourColor)
            elif(key=="colorMinute"):
                value=self.getText(self.textHandleHourColor)
                mySettingsDict[key]=value
            elif(key=="brightness"):
                mySettingsDict[key]=self.getText(self.textBrightness)
            elif (key=="tickColor"):
                mySettingsDict[key]=self.getText(self.textTickColor)
            elif (key=="dimOn"):
                mySettingsDict[key]=self.checkbuttonAutoDim_var.get()
            elif(key=="ValueDim"):
                mySettingsDict[key]=self.getText(self.textValueDim)
            elif(key=="hourStartDim"):
                mySettingsDict[key]=self.spinAutoDimStartValue.get()
            elif(key=="hourStopDim"):
                mySettingsDict[key]=self.spinAutoDimStopValue.get()

        pass

    def on_sendUpdate_clicked(self):
        self.updateDictSettings()
        liste=(mySettingsDict.keys)
        self.UpdateForm = UpdateForm(self, listvar=liste, master=self.root)
        pass

    def onValueDimClicked(self):
        helligkeit=self.textValueDim.get("1.0","end")
        self.helligkeitsForm = Helligkeit(self, helligkeit, self.textValueDim, self.root)
        pass

    def on_buttonHelligkeit(self):
        helligkeit=self.textBrightness.get("1.0","end")
        self.helligkeitsForm = Helligkeit(self, helligkeit, self.textBrightness, self.root)
        pass

    def on_buttonTickColor(self):
        #in 0x323232 (50,50,50)
        iRGB=int(self.textTickColor.get(1.0,"end"))
        tkRGB=self.int2RGBstr(iRGB)

        (tkColorTuple, tkColorStr)=self.selectColorDialog(titel="Ticks Farbe", color=tkRGB) #TODO
        #out 320000
        if(tkColorStr != None):
            iRGB=self.getIntFromTuple(tkColorTuple=tkColorTuple)
            self.setText(self.textTickColor, str(iRGB))

        #tickColor=self.textTickColor.get("1.0","end")
        #self.settings = FormColor(self.update, self, tickColor, self.textTickColor, self.root)
        #self.helligkeitsForm = Helligkeit(self, helligkeit, self.textBrightness, self.root)
        pass

    def getColorHour(self):
        iRGB=int(self.textHandleHourColor.get(1.0,"end"))
        tkRGB=self.int2RGBstr(iRGB)

        (tkColorTuple, tkColorStr)=self.selectColorDialog(titel="Stunden Zeiger Farbe", color=tkRGB) #TODO
        if(tkColorStr != None):
            iRGB=self.getIntFromTuple(tkColorTuple=tkColorTuple)
            self.setText(self.textHandleHourColor, str(iRGB))
        #self.settings = FormColor(self.update, self, self.textHandleHourColor.get("1.0","end"), self.textHandleHourColor, self.root)
        pass

    def getColorMinute(self):
        iRGB=int(self.textHandleMinuteColor.get(1.0,"end"))
        tkRGB=self.int2RGBstr(iRGB)

        (tkColorTuple, tkColorStr)=self.selectColorDialog(titel="Minuten Zeiger Farbe", color=tkRGB) #TODO
        if(tkColorStr != None):
            iRGB=self.getIntFromTuple(tkColorTuple=tkColorTuple)
            self.setText(self.textHandleMinuteColor, str(iRGB))
        #self.settings = FormColor(self.update, self, self.textHandleMinuteColor.get("1.0","end"), self.textHandleMinuteColor, self.root)
        pass

    def getColorSecond(self):
        iRGB=int(self.textHandleSecondColor.get(1.0,"end"))
        tkRGB=self.int2RGBstr(iRGB)

        (tkColorTuple, tkColorStr)=self.selectColorDialog(titel="Sekunden Zeiger Farbe", color=tkRGB) #TODO
        if(tkColorStr != None):
            iRGB=self.getIntFromTuple(tkColorTuple=tkColorTuple)
            self.setText(self.textHandleSecondColor, str(iRGB))
        #self.settings = FormColor(self.update, self, self.textHandleSecondColor.get("1.0","end"), self.textHandleSecondColor, self.root)
        pass

    def sendSavePrefs(self):
        self.mqtt_client.publish("/esp32_clock/save_Pref", "1")
        pass
    def sendLoadPrefs(self):
        self.mqtt_client.publish("/esp32_clock/load_Pref", "1")
        self.mqtt_client.publish("/esp32_clock/dumpValues", "1")
        pass

    def rgb_to_hex(self, r, g, b):
        s='#' + ''.join('{:02X}'.format(value) for value in (r, g, b))
        s=s[0:7]
        return s

    # convert int to tk hex color
    def int2RGB(self, i):
        r=i & 0xff0000
        g=i & 0x00ff00
        b=i & 0x0000ff

        return self.rgb_to_hex(r,g,b)
        pass
    
    def int2RGBstr(self,i):
        r=i & 0xff0000
        g=i & 0x00ff00
        b=i & 0x0000ff
        tkColor='#' + format(i, '06x')
        return tkColor
#        return self.rgb_to_hex(r,g,b)    
        pass

    def getRGB(self):
        R=self.Color_R.get()
        G=self.color_G.get()
        B=self.color_B.get()

        self.rgbStr=self.rgb_to_hex(R, G, B)
        self.colorRGB=self.rgbStr
        return self.rgbStr
        
    def getRGBint(self):
        R=self.Color_R.get()
        G=self.color_G.get()
        B=self.color_B.get()
        i = R * 0xff00 + G * 0x00ff + B 
        return i
        pass
    def getIntFromTuple(self, tkColorTuple):
        # (r,g,b)
        str='%02x%02x%02x' % tkColorTuple
        iColor=int(str,16)
        return iColor
        pass

    # show colorchooser, in Title string and tkColor preset
    def selectColorDialog(self, color:str, titel:str):
        c_code=tk.colorchooser.askcolor(title=titel, color=color)
        print("colorchooser: ")
        print(c_code) # returns a tuple of (r,g,b) and the tk color string #0a1fcc
        # ((145, 217, 217), '#91d9d9')
        (rgbTuple, tkColor)=c_code
        return (rgbTuple, tkColor)
        pass
'''    
class FormColor:
    def __init__(self, func, callerWin, sRGB, callerText, master):
        self.win=tk.Toplevel(master)
        self.win.resizable(False, False)
        
        self.func=func 
        self.caller=callerWin
        self.callerText=callerText
        
        self.rgbStr=sRGB

        self.win.title("Settings")
        self.win.geometry("520x380")
        root=self.win

        self.var = tk.IntVar()
        self.OkCancel=False
        self.colourR=0.0
        self.colourG=0.0
        self.colourB=0.0
        self.colorRGB="" # (self.colourR << 8) + (self.colourG << 16) + self.colourB

        self.Color_R = tk.Scale(root, from_=0, to=255, orient="horizontal", command=self.on_change_R)
        self.Color_R.place(x=20, y=20, width=180, height=48)

        self.color_G = tk.Scale(root, from_=0, to=255, orient="horizontal", command=self.on_change_G)
        self.color_G.place(x=20, y=90, width=180, height=48)

        self.color_B = tk.Scale(root, from_=0, to=255, orient="horizontal", command=self.on_change_B)
        self.color_B.place(x=20, y=160, width=180, height=48)

        self.buttonOK = tk.Button(root, text="OK", command=self.on_buttonOK)
        self.buttonOK.place(x=310, y=250, width=96, height=32)

        self.buttonCancel = tk.Button(root, text="Cancel", command=self.on_buttonCancel)
        self.buttonCancel.place(x=190, y=250, width=96, height=32)

        self.textColor = tk.Text(root, bg="#f6d32d")
        self.textColor.place(x=286, y=20, width=120, height=100)

        self.label=tk.Label(root, text="Setting Window")
        self.label.place(x=20, y=286, width=120, height=100)

        #set scales to R G B
        self.setScale(self.Color_R, int(sRGB) & 0xff0000)
        self.setScale(self.color_G, int(sRGB) & 0x00ff00)
        self.setScale(self.color_B, int(sRGB) & 0x0000ff)

        #set bg of textColor
        self.setText(self.textColor, self.int2RGBstr(int(sRGB)))
        self.textColor.config(bg=self.int2RGBstr(int(sRGB)))

        #TEST
        i=self.getRGBint()

        pass

    def setText(self, txtObj, txt):
        txtObj.delete(1.0, "end")
        txtObj.insert(1.0, txt)
        pass

    def setScale(self, scaleObj, val):
        scaleObj.set(val)
        pass

    def ParentFunc(self):
        self.func(self.var.get())

    def on_buttonCancel(self):
        self.win.destroy()
        pass

    def rgb_to_hex(self, r, g, b):
        s='#' + ''.join('{:02X}'.format(value) for value in (r, g, b))
        s=s[0:7]
        return s

    def int2RGB(self, i):
        r=i & 0x00ff0000
        g=i & 0x0000ff00
        b=i & 0x000000ff

        return self.rgb_to_hex(r,g,b)
        pass

    def int2RGBstr(self,i):
        r=i & 0x00ff0000
        g=i & 0x0000ff00
        b=i & 0x000000ff
        return self.rgb_to_hex(r,b,g)    
        pass

    def getRGB(self):
        R=self.Color_R.get()
        G=self.color_G.get()
        B=self.color_B.get()

        self.rgbStr=self.rgb_to_hex(R, G, B)
        self.colorRGB=self.rgbStr
        return self.rgbStr
        
    def getRGBint(self):
        R=self.Color_R.get()
        G=self.color_G.get()
        B=self.color_B.get()
        i = R * 0xff00 + G * 0x00ff + B 
        return i
        pass

    def on_buttonOK(self):
        self.caller.setText(self.callerText, self.getRGBint())
        self.win.destroy()
        pass

    def returnColor(self):
        return self.getRGBint()
        pass

    def on_change_R(self, value):
        self.colourR=value
#        self.caller.colorR=value
#        print(f"color in main: {self.caller.colorR}")
        self.updateColorShow()
        pass

    def on_change_G(self, value):
        self.colourG=value
        self.updateColorShow()
        pass

    def on_change_B(self, value):
        self.colourB=value
        self.updateColorShow()
        pass

    def updateColorShow(self):
        newColor=self.getRGB()
        self.textColor.config(bg=newColor);
        self.setText(self.textColor, newColor)
        pass
'''

class Helligkeit:
    def __init__(self, callerWin, helligkeit, callerText, master):
        self.win=tk.Toplevel(master)
        self.win.title("Helligkeit")
        self.win.geometry("520x380")
        self.win.resizable(False, False)
        root=self.win
        self.callerText=callerText

        self.helligkeit=helligkeit
        self.caller=callerWin

        self.label1 = tk.Label(root, text="Helligkeit:")
        self.label1.place(x=30, y=20, width=90, height=26)

        self.scale1 = tk.Scale(root, from_=0, to=255, orient="horizontal", command=self.on_change)
        self.scale1.place(x=60, y=70, width=180, height=48)

        self.buttonOK = tk.Button(root, text="OK", command=self.button_OK_clicked)
        self.buttonOK.place(x=380, y=300, width=96, height=32)

        self.canvas1ShowBrightness = tk.Canvas(root, bg="#000000", highlightthickness=1, highlightbackground="#828282")
        self.canvas1ShowBrightness.place(x=308, y=46, width=120, height=90)
        
        self.buttonCancel = tk.Button(root, text="Cancel", command=self.button_Cancel_clicked)
        self.buttonCancel.place(x=260, y=304, width=96, height=32)
#        self.buttonCancel.bind("<Button-1>", self.on_buttonCancel_button_1)

        #set scale value
        self.scale1.set(self.helligkeit)
        pass

    def button_OK_clicked(self):
#        self.callback(self.helligkeit)
        # get scale value

        self.caller.setText(self.callerText, str(self.scale1.get()))
        self.win.destroy()
        pass

    def button_Cancel_clicked(self):
        self.win.destroy()
        pass

    def setScale(self, scaleObj, val):
        scaleObj.set(val)
        pass

    def rgb_to_hex(self, r, g, b):
        s='#' + ''.join('{:02X}'.format(value) for value in (r, g, b))
        s=s[0:7]
        return s
    
    def on_change(self, value):
        v=int(value)
        s=self.rgb_to_hex(v, v, v)
        #self.setScale(self.canvas1ShowBrightness, s)
        self.canvas1ShowBrightness.config(background=s)
        pass

#update form
class UpdateForm:
    def __init__(self, callerWin, listvar, master):
        self.win = tk.Toplevel(master)
        root=self.win
        self.win.title("Update")
        self.win.geometry("520x380")
        self.win.resizable(False, False)


        self.mqtt = MQTTClient("192.168.0.40", 1883, self)
        self.mqtt.connect()
        
        global mySettingsDict
        list_variable=mySettingsDict.keys()

        frame=tk.Frame(self.win)
        frame.pack(side="left", padx=20, pady=20, anchor="nw")
        
        self.listbox1 = tk.Listbox(frame, selectmode="multiple", listvariable=list_variable)
        self.listbox1.pack(side="left",fill="y")

        scrollbar = tk.Scrollbar(frame, orient="vertical")
        scrollbar.config(command=self.listbox1.yview)
        scrollbar.pack(side="right", fill="y")
        
        self.listbox1.config(yscrollcommand=scrollbar.set)

        #self.listbox1 = tk.Listbox(root, selectmode="multiple", listvariable=list_variable)
        for item in mySettingsDict.keys():
            self.listbox1.insert("end", item)
        '''
        self.listbox1.insert("end", "Item 1")
        self.listbox1.insert("end", "Item 2")
        self.listbox1.insert("end", "Item 3")
        '''
        #self.listbox1.place(x=20, y=23, width=160, height=110)
        self.listbox1.bind('<<ListboxSelect>>', self.handle_item_select)

        scrollbar=tk.Scrollbar(self.listbox1, orient="vertical")
        scrollbar.config(command=self.listbox1.yview)

        self.buttonUpdate = tk.Button(root, text="Send Update", command=self.on_sendUpdate_clicked)
        self.buttonUpdate.place(x=282, y=20, width=96, height=32)
        self.win.protocol("WM_DELETE_WINDOW", self.on_closing)

    def on_closing(self):
        print("UpdateForm closing called")
        self.mqtt.disconnect()
        del self.mqtt
        self.win.destroy()
        pass

    def __del__(self):
        print("UpdateForm delete called")
        self.mqtt.disconnect()
        del self.mqtt
        pass

    #needed for MQTT class    
    def updateDumpTxt(self, text):
        print("UpdateForm: updateDumpText" + text)
        pass

    def handle_item_select(self, event):
        selected_indices = self.listbox1.curselection()
        selected_items = ",".join([self.listbox1.get(i) for i in selected_indices])
        print("Selected: " + selected_items)
        '''
        showinfo(
            title='Information', 
            message=f'You selected: {selected_languages}'
        )
        '''
    def on_sendUpdate_clicked(self):
        selected_indices = self.listbox1.curselection()
        for i in selected_indices:
            k=self.listbox1.get(i)
            topic=getTopicForKey(k)
            payload=str(mySettingsDict.get(k))
            if(payload.endswith("\n")):
                payload=payload.strip("\n")
            payload=payload.strip()
            self.mqtt.publish("/esp32_clock/" + topic, payload)
            print("updating " + topic + " to " +str(mySettingsDict.get(k)))
            
        pass

if __name__ == "__main__":
    root = tk.Tk()
    app = Main(root)
    root.resizable(False,False)
    root.mainloop()
