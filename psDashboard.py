# Designed with https://visualtkinter.com - paste this file into File > Import there to edit it visually
import tkinter as tk
from tkinter import ttk
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
        
    def publish(self, topic, message):
        result = self.client.publish(topic, message)
        return result

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        
class Main:
    def __init__(self, root):
        mqtt_client = MQTTClient(broker, port, self)
        mqtt_client.connect()
        msg=1
        result = mqtt_client.publish(topic_pub, "1") #request settings
        # result: [0, 1]
        status = result[0]
        if status == 0:
                print(f"Send `{msg}` to topic `{topic_pub}`")
        else:
                print(f"Failed to send message to topic {topic_pub}")

        self.root = root
        root.title("Main")
        root.geometry("520x380")

        self.settings = None
        self.SettingsValue = None

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

        self.buttonFarbeHour = tk.Button(root, text="Farbe", command=self.getColorHour)
        self.buttonFarbeHour.place(x=230, y=174, width=96, height=32)

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

    def setText(self, txtObj, txt):
        txtObj.delete(1.0, "end")
        txtObj.insert(1.0, txt)

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

        self.setText(self.textHandleHourColor, str(y["handleHourColor"]))
        self.setText(self.textHandleMinuteColor, str(y["handleMinuteColor"]))
        self.setText(self.textHandleSecondColor, str(y["handleSecondColor"]))
        pass

    def update(self, n):
        self.SettingsValue = n
        print(self.SettingsValue)
  
    def openSettings(self):
        self.settings = Form2(self.update, root)
  
    def updateLabelinSettings(self):
        #prompt = askinteger("Input", "Input an Integer")
        if self.settings != None:
            self.settings.label.configure(text="Updated")

    def open_form_2(self):
        return Form2(self.root)

    def dumpSettings(self):
        pass

    def getColorHour(self):
        #inputDialog = Form2(root)
        #root.wait_window(inputDialog.top)
        self.settings = Form2(self.update, self, self.textHandleHourColor.get("1.0","end"), self.textHandleHourColor)
        pass

    def getColorMinute(self):
        self.settings = Form2(self.update, self, self.textHandleMinuteColor.get("1.0","end"), self.textHandleMinuteColor)
        pass

    def getColorSecond(self):
        self.settings = Form2(self.update, self, self.textHandleSecondColor.get("1.0","end"), self.textHandleSecondColor)
        pass


class Form2:
    def __init__(self, func, callerWin, sRGB, callerText): #master):
        self.win=tk.Toplevel()
        self.func=func 
        self.caller=callerWin
        self.callerText=callerText
        
        self.rgbStr=sRGB

        #root = tk.Toplevel(master)
        #self.root = root
        self.win.title("Settings")
        self.win.geometry("520x380")
        root=self.win
        #root.title("Form 2")
        #root.geometry("520x380")

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

if __name__ == "__main__":
    root = tk.Tk()
    app = Main(root)
    root.mainloop()
