import sys
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from PyQt5.QtWebEngineWidgets import QWebEngineView

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.resize(800, 600)
        self.setWindowTitle("Browser Tab Example")

        vbox = QVBoxLayout()
        self.setLayout(vbox)

        self.tab_widget = QTabWidget()

        # Set font size for tab widget
        tab_widget_font = QFont()
        tab_widget_font.setPointSize(14)
        self.tab_widget.setFont(tab_widget_font)

        # Browser tab
        browser_tab = QWidget()
        self.tab_widget.addTab(browser_tab, "Browser")
        vbox_browser = QVBoxLayout()
        browser_tab.setLayout(vbox_browser)

        # Address bar
        self.address_bar = QLineEdit()
        self.address_bar.setPlaceholderText("Enter a URL")
        self.address_bar.returnPressed.connect(self.load_url)

        vbox_browser.addWidget(self.address_bar)

        # Create browser view
        self.browser_view = QWebEngineView()
        self.browser_view.load(QUrl("https://www.baidu.com"))  # Change the URL to the desired website

        vbox_browser.addWidget(self.browser_view)

        vbox.addWidget(self.tab_widget)

    def load_url(self):
        url = self.address_bar.text()
        if not url.startswith("http://") and not url.startswith("https://"):
            url = "http://" + url
        self.browser_view.load(QUrl(url))

def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
