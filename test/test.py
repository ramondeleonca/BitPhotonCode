import websocket

def on_message(ws, message):
    print(message)

def on_open(ws):
    print("### opened ###")

if __name__ == "__main__":
    # websocket.enableTrace(True)
    ws = websocket.WebSocketApp("ws://192.168.1.96:81", on_message = on_message)
    ws.on_open = on_open
    ws.run_forever()