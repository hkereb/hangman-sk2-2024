import sys
from ui_skeleton import Ui_MainWindow
from PySide6.QtWidgets import QApplication, QMainWindow
from PySide6.QtCore import QObject, Signal, QThread, QRegularExpression
from PySide6.QtGui import QRegularExpressionValidator

def substr_msg(msg):
    return msg[3:]

class MainApp(QMainWindow):
    nick_submitted = Signal(str)  # sygnał do przesyłania nicku
    update_rooms = Signal(str) # sygnał informujący serwer że ma wysłać aktualną listę pokoi

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # strona startowa
        self.ui.stackedWidget.setCurrentWidget(self.ui.nick_page)
        #self.ui.stackedWidget.setCurrentWidget(self.ui.create_or_join_page)

        # walidator adresu IP
        ip_regex = QRegularExpression(r'^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$')
        validator = QRegularExpressionValidator(ip_regex)
        self.ui.create_IP_field.setValidator(validator)

        self.ui.level_combobox.addItems(["easy", "medium", "hard", "extra hard"])

        # connect
        self.ui.nick_submit_btn.clicked.connect(self.submit_nick)
        self.ui.create_IP_field.textChanged.connect(self.on_ip_changed)
        self.ui.stackedWidget.currentChanged.connect(self.is_at_create_or_join_page)
        self.ui.rooms_list.itemSelectionChanged.connect(self.on_list_item_selected)

    def submit_nick(self):
        nick = self.ui.nick_field.text()
        if nick:
            self.nick_submitted.emit(nick)
            print(f"Nick submitted: {nick}")

    def handle_server_response(self, message):
        if message.startswith("01"):
            result = substr_msg(message)
            if result == "1":  # nick zaakceptowany
                self.ui.stackedWidget.setCurrentWidget(self.ui.create_or_join_page)
            elif result == "0":  # nick odrzucony
                self.ui.check_label.setText("\U0000274C")
                self.ui.check_label.setStyleSheet("color: red;")
        elif message.startswith("04"):
            lobbies_encoded = substr_msg(message)
            lobbies = lobbies_encoded.split(",")

            self.ui.rooms_list.clear()

            for lobby in lobbies:
                self.ui.rooms_list.addItem(lobby)

            print("Lista pokoi została zaktualizowana.")

    def on_ip_changed(self):
        if self.ui.create_IP_field.hasAcceptableInput():
            self.ui.check_ip_label.setText("\U00002714")
            self.ui.check_ip_label.setStyleSheet("color: green;")
        else:
            self.ui.check_ip_label.setText("\U0000274C")
            self.ui.check_ip_label.setStyleSheet("color: red;")

    def is_at_create_or_join_page(self, index):
        if self.ui.stackedWidget.widget(index) == self.ui.create_or_join_page:
            self.update_rooms.emit("")

    def on_list_item_selected(self):
        selected_item = self.ui.rooms_list.currentItem()
        if selected_item:
            self.ui.join_room_name_field.setText(selected_item.text())