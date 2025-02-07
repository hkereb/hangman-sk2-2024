import sys
import client
from client import NetworkClient
###################################
from gui import MainApp
from PySide6 import QtWidgets as qtw
from PySide6 import QtCore as qtc

def try_connect(ip):
    if not client.isConnected:
        client.server_ip = ip
        client.server_port = SERVER_PORT
        client.connect_to_server()
    else:
        window.submit_nick()

def handle_cant_connect(error_message):
    qtw.QMessageBox.critical(window, "Connection Error", f"Cannot connect to the server:\n{error_message}")

def handle_connection_error(error_message):
    window.clean_upon_disconnect()
    qtw.QMessageBox.critical(window, "Connection Error",f"The connection with the server has been lost:\n{error_message}")

def unlock_nickname():
    window.submit_nick()

def disconnect_from_server():
    if client.isConnected:
        window.clean_upon_disconnect()
        client.intentional_disconnect = True
        client.disconnect()
        qtw.QMessageBox.information(window, "Disconnected", "You have been disconnected from the server.")

if __name__ == "__main__":
    #SERVER_IP = "192.168.100.8"
    SERVER_PORT = 1111

    client = NetworkClient(None, None, None)

    app = qtw.QApplication([])

    window = MainApp()
    window.show()

    ############# signals ##############
    client.message_received.connect(window.handle_server_response)
    window.sig_submit_nick.connect(lambda nick: client.send_to_server("01", f"{nick}"))
    window.sig_create_room.connect(lambda name, password, level, rounds, time_sec: client.send_to_server("02",f"name:{name},password:{password},difficulty:{level},rounds:{rounds},time:{time_sec}"))
    window.sig_join_room.connect(lambda name, password: client.send_to_server("03", f"name:{name},password:{password}"))
    window.sig_rooms_list.connect(lambda message: client.send_to_server("70", ""))
    window.sig_players_list.connect(lambda message: client.send_to_server("71", ""))
    window.sig_start.connect(lambda message: client.send_to_server("73", ""))
    window.sig_submit_letter.connect(lambda letter: client.send_to_server("06", f"{letter}"))
    window.sig_time_ran_out.connect(lambda message: client.send_to_server("80", ""))
    window.sig_leave_room.connect(lambda message: client.send_to_server("09", ""))
    window.sig_ready_to_play.connect(lambda message: client.send_to_server("82", message))

    client.sig_cant_connect.connect(lambda error_msg: handle_cant_connect(error_msg))
    client.sig_server_disconnected.connect(lambda error_msg: handle_connection_error(error_msg))
    window.sig_connect.connect(try_connect)
    window.sig_has_connected.connect(unlock_nickname)
    window.sig_disconnect_me.connect(disconnect_from_server)
    ####################################

    # event loop
    sys.exit(app.exec())