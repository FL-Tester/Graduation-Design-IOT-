import sys
import json
import pymysql
import time
from datetime import datetime
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
import paho.mqtt.client as mqtt
from PyQt5.QtWebEngineWidgets import QWebEngineView

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        # Simple in-memory database for demonstration purposes
        self.db = {
            "temperature": [],
            "humidity": [],
            "light_intensity": [],
            "soil_moisture": [],
            "servo_position": []
        }
        self.initUI()
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_message = self.on_mqtt_message
        self.mqtt_client.connect("175.178.79.144", 1883, 60)
        self.mqtt_client.loop_start()

    def initUI(self):
        self.resize(800, 600) 
        self.setWindowTitle("校园林木自动灌溉系统")
        vbox = QVBoxLayout()
        self.setLayout(vbox)

        self.tab_widget = QTabWidget()
        # Set font size for tab widget
        tab_widget_font = QFont()
        tab_widget_font.setPointSize(20)
        self.tab_widget.setFont(tab_widget_font)

        # Monitoring tab
        monitoring_tab = QWidget()
        self.tab_widget.addTab(monitoring_tab, "节点监控")
        vbox_monitoring = QVBoxLayout()
        monitoring_tab.setLayout(vbox_monitoring)
        labels = ["温度:", "湿度:", "光照强度:", "土壤湿度:", "水泵档位:"]
        self.data_values = []
        for i in range(2):
            group_box = QGroupBox(f"节点 {i + 1}")
            # Set font size for group box
            group_box_font = QFont()
            group_box_font.setPointSize(20)
            group_box.setFont(group_box_font)
            vbox_node = QVBoxLayout()
            node_data_values = []
            for label in labels:
                hbox = QHBoxLayout()
                data_label = QLabel(label)
                # Set font size for label
                label_font = QFont()
                label_font.setPointSize(18)
                data_label.setFont(label_font)
                data_value = QLabel("0")  # Initialize data with 0
                # Set font size for data value
                data_value_font = QFont()
                data_value_font.setPointSize(18)
                data_value.setFont(data_value_font)
                hbox.addWidget(data_label)
                hbox.addWidget(data_value)
                vbox_node.addLayout(hbox)
                node_data_values.append(data_value)
            group_box.setLayout(vbox_node)
            vbox_monitoring.addWidget(group_box)
            self.data_values.append(node_data_values)
        group_box.setLayout(vbox_node)
        vbox_monitoring.addWidget(group_box)
        self.data_values.append(node_data_values)

        #水泵开关
        self.servo_control_panel = QGroupBox("水泵控制")
        self.servo_control_panel.setFont(group_box_font)
        vbox_servo_control = QVBoxLayout()
        self.servo_control_panel.setLayout(vbox_servo_control)
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["自动模式", "手动模式"])
        vbox_servo_control.addWidget(self.mode_combo)

        self.node_combo = QComboBox()
        self.node_combo.addItems(["终端节点1", "终端节点2"])
        vbox_servo_control.addWidget(self.node_combo)

        self.switch_combo = QComboBox()
        self.switch_combo.addItems(["开", "关"])
        vbox_servo_control.addWidget(self.switch_combo)

        self.execute_button = QPushButton("执行")
        self.execute_button.clicked.connect(self.execute_settings)
        vbox_servo_control.addWidget(self.execute_button)
        vbox_monitoring.addWidget(self.servo_control_panel)

        # Database query tab
        db_query_tab = QWidget()
        self.tab_widget.addTab(db_query_tab, "数据库")
        vbox_db_query = QVBoxLayout()
        db_query_tab.setLayout(vbox_db_query)
        self.data_table = QTableWidget()
        vbox_db_query.addWidget(self.data_table)
        self.update_button = QPushButton("更新")
        self.delete_button = QPushButton("删除")
        self.update_button.clicked.connect(self.query_and_display_data)
        self.delete_button.clicked.connect(self.delete_data)
        vbox_db_query.addWidget(self.update_button)
        vbox_db_query.addWidget(self.delete_button)
        vbox.addWidget(self.tab_widget)
        self.query_and_display_data()

    def load_url(self):
        url = self.address_bar.text()
        if not url.startswith("http://") and not url.startswith("https://"):
            url = "http://" + url
        self.browser_view.load(QUrl(url))
    def on_mqtt_connect(self, client, userdata, flags, rc):
        print("Connected to MQTT broker with result code: " + str(rc))
        self.mqtt_client.subscribe("/CD/MCU1/ESP32")
    def query_and_display_data(self):
        db = pymysql.connect(host='175.178.79.144',
                             user='root',
                             password='dnFjfpw77zYyNA6Z',
                             database='cd_db')
        cursor = db.cursor()
        # Replace "your_table_name" with the actual name of your table
        query = "SELECT node, temp, humi, light, wp, soilhumi, time FROM FL_GP"
        cursor.execute(query)
        results = cursor.fetchall()
        # Set the table headers
        self.data_table.setColumnCount(7)
        self.data_table.setHorizontalHeaderLabels(["节点", "温度", "湿度", \
                                                   "光照强度", "水泵挡位", "土壤湿度", "时间"])
        # Set the number of rows based on the number of results
        self.data_table.setRowCount(len(results))
        for row_num, row_data in enumerate(results):
            for col_num, col_data in enumerate(row_data):
                self.data_table.setItem(row_num, col_num, QTableWidgetItem(str(col_data)))
        cursor.close()
        db.close()
    def on_mqtt_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode("utf-8")
        print (topic, payload)
        try:
            data = json.loads(payload)
            # Extract data from the JSON object
            if data.get("device") == "node1":
                node = 1
            elif data.get("device") == "node2":
                node = 2   
            temp = data.get("temperature")
            humi = data.get("humidity")
            light = data.get("light")
            wp = data.get("water_pump")
            soilhumi = data.get("soil_humi")
            # Insert data into the database
            self.insert_data_into_db(node, temp, humi, light, wp, soilhumi)
            # Update the data values in the GUI
            self.data_values[node-1][0].setText(str(temp))
            self.data_values[node-1][1].setText(str(humi))
            self.data_values[node-1][2].setText(str(light))
            self.data_values[node-1][3].setText(str(soilhumi))
            self.data_values[node-1][4].setText(str(wp))
    
        except json.JSONDecodeError:
            print(f"Failed to decode JSON message: {payload}")
    def insert_data_into_db(self, node, temp, humi, light, wp, soilhumi):
        db = pymysql.connect(host='175.178.79.144',
                            user='root',
                            password='dnFjfpw77zYyNA6Z',
                            database='cd_db')
        cursor = db.cursor()

        sql = "INSERT INTO %s (node, temp, humi, light, wp, soilhumi ,  \
            time)VALUES ('%s', '%s', '%s', '%s', '%s', '%s',            \
            '%s')" % ('FL_GP', node, temp, humi, light, wp,             \
            soilhumi, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
        cursor.execute(sql)
        db.commit()
        cursor.close()
        db.close()
    def delete_data(self):
        db = pymysql.connect(host='175.178.79.144',
                            user='root',
                            password='dnFjfpw77zYyNA6Z',
                            database='cd_db')
        cursor = db.cursor()           
        sql = "DELETE FROM %s" % ("FL_GP")
        cursor.execute(sql)
        ## id 重置
        sql = "ALTER TABLE %s AUTO_INCREMENT = 1" % ("FL_GP")
        cursor.execute(sql)
        db.commit()
        cursor.close()
        db.close()              
    def execute_settings(self):
        if self.mode_combo.currentText() == "手动模式":
            mode = 1
        else:
            mode = 0
        if self.switch_combo.currentText() == "开":
            switch = 1
        else:
            switch = 0
        if self.node_combo.currentText() == "终端节点1":
            node = 1
        else:
            node = 2
        message = {
            "mode": mode,
            "switch": switch,
            "node": node
        }
        self.mqtt_client.publish("/CD/SWJ", json.dumps(message))
def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
