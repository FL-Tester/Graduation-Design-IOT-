import sys
import pymysql
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.resize(800, 600)
        self.setWindowTitle("Nodes Data Display")

        vbox = QVBoxLayout()
        self.setLayout(vbox)

        self.tab_widget = QTabWidget()

        # Set font size for tab widget
        tab_widget_font = QFont()
        tab_widget_font.setPointSize(14)
        self.tab_widget.setFont(tab_widget_font)

        # Database query tab
        db_query_tab = QWidget()
        self.tab_widget.addTab(db_query_tab, "Database Query")
        vbox_db_query = QVBoxLayout()
        db_query_tab.setLayout(vbox_db_query)

        self.data_table = QTableWidget()
        vbox_db_query.addWidget(self.data_table)

        self.update_button = QPushButton("Update Data")
        self.update_button.clicked.connect(self.query_and_display_data)
        vbox_db_query.addWidget(self.update_button)

        vbox.addWidget(self.tab_widget)

        self.query_and_display_data()

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
        self.data_table.setHorizontalHeaderLabels(["节点", "温度", "湿度", "光照强度", "水泵挡位", "土壤湿度", "时间"])

        # Set the number of rows based on the number of results
        self.data_table.setRowCount(len(results))

        for row_num, row_data in enumerate(results):
            for col_num, col_data in enumerate(row_data):
                self.data_table.setItem(row_num, col_num, QTableWidgetItem(str(col_data)))

        cursor.close()
        db.close()

def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
