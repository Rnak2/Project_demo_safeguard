import sys
import paho.mqtt.client as mqtt
from PyQt5.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QLabel
from PyQt5.QtCore import pyqtSlot, QTimer

MQTT_BROKER = "192.168.86.182" 
MQTT_PORT = 1883
MQTT_TOPIC_COMMAND = "safeguard/command"
MQTT_TOPIC_ALERT = "safeguard/alert"

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

        self.timer = QTimer()
        self.timer.timeout.connect(self.client.loop)
        self.timer.start(100)

        self.init_ui()
        self.connect_mqtt()

    def init_ui(self):
        self.setWindowTitle("Safeguard System")
        layout = QVBoxLayout()
        widget = QWidget(self)
        self.setCentralWidget(widget)
        widget.setLayout(layout)

        self.label = QLabel("Controller")
        layout.addWidget(self.label)

        lock_button = QPushButton('Lock')
        lock_button.clicked.connect(lambda: self.send_command('lock'))
        layout.addWidget(lock_button)

        unlock_button = QPushButton('Unlock')
        unlock_button.clicked.connect(lambda: self.send_command('unlock'))
        layout.addWidget(unlock_button)

        toggle_button = QPushButton('Toggle Safe Mode')
        toggle_button.clicked.connect(lambda: self.send_command('toggle_safe_mode'))
        layout.addWidget(toggle_button)

    def connect_mqtt(self):
        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.label.setText("Connecting to MQTT...")
            print("Connecting to MQTT broker...")
        except Exception as e:
            self.label.setText(f"Connection failed: {e}")
            print(f"Connection failed: {e}")
            QTimer.singleShot(5000, self.connect_mqtt)  

    def send_command(self, command):
        self.client.publish(MQTT_TOPIC_COMMAND, command)
        print(f"Published command: {command}")

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.label.setText("Connected to MQTT")
            print("Connected to MQTT broker")
            self.client.subscribe(MQTT_TOPIC_ALERT)
        else:
            self.label.setText(f"Failed to connect, return code {rc}")
            print(f"Failed to connect, return code {rc}")
            QTimer.singleShot(5000, self.connect_mqtt)  

    def on_disconnect(self, client, userdata, rc):
        self.label.setText("Disconnected from MQTT")
        print("Disconnected from MQTT broker")
        QTimer.singleShot(5000, self.connect_mqtt)  

    def on_message(self, client, userdata, msg):
        self.label.setText(f"Alert: {msg.payload.decode()}")
        print(f"Received message: {msg.payload.decode()}")

app = QApplication(sys.argv)
window = MainWindow()
window.show()
app.exec_()
