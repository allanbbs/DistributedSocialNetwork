import os
import sys
import json
from datetime import datetime
import threading
import socket
from PySide6 import QtCore
from PySide6.QtCore import QSize, Qt, QThread
from PySide6.QtWidgets import (QGridLayout, QLabel, QLayout, QLineEdit, QPushButton, QApplication, QHBoxLayout,
                               QDialog, QListWidget, QListWidgetItem, QSizePolicy, QTreeWidget, QTreeWidgetItem, QTreeWidgetItemIterator, QVBoxLayout, QWidget)



def bin_to_hex(bin):
    return hex(int(bin, 2))

class Worker(QThread):
    clear_signal = QtCore.Signal()
    pull_signal = QtCore.Signal(tuple)

    def __init__(self, sock, parent=None):
        self.socket = sock
        self.is_running = False
        QtCore.QThread.__init__(self, parent)

    def receive_data(self):
        data = b"{}"
        size = int.from_bytes(self.socket.recv(2), "little")
        if size == 0:
            self.is_running = False
            return json.loads(data.decode('utf-8'))
        data = self.socket.recv(size)
        if data.decode('utf-8') == "null":
            data = b"{}"
        return json.loads(data.decode('utf-8'))

    def run(self):
        self.is_running = True
        while self.is_running:
            timeline = self.receive_data()
            self.clear_signal.emit()
            msgs = []
            for (uuid, messages) in timeline.items():
                for msg in messages:
                    msgs.append(msg)
            msgs = sorted(msgs, key=lambda x: x["timestamp"])
            for m in msgs:
                dt = datetime.fromtimestamp(int(m["timestamp"]))
                self.pull_signal.emit((m["body"], str(dt)[:-3],str(bin_to_hex(m["sender"]))))


class Form(QDialog):

    def __init__(self, parent=None):
        super(Form, self).__init__(parent)
        self.is_running = True
        self.mutex = threading.Lock()

        # Start socket
        self.socket_sender = None
        self.socket_receiver = None
        self.start_socket()

        # Get peer info
        self.id = self.bin_to_hex(self.send_command(self.create_command("GET_ID"), True)["value"])
        print("My ID: ", self.id)
        subscribers = self.send_command(self.create_command("GET_SUBSCRIBERS"), True)

        # Create widgets

        # Timeline
        self.id_label = QLabel()
        self.id_label.setText('<h3 style="color:white">%s</h3>' % self.id)
        self.id_label .setTextInteractionFlags(Qt.TextSelectableByMouse)

        self.list = QListWidget()
    
        #Followers 
        self.followers = QTreeWidget()
        self.followers.setSizePolicy(QSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored))
        self.followers.setHeaderLabel("Followed Peers")
        self.followers.setStyleSheet("background-color: #b2beb5; border-radius: 15px; outline: 0;")
        self.followers.header().setDefaultAlignment(Qt.AlignCenter)
        self.followers.setRootIsDecorated(False)

        for sub in subscribers['subscribers']:
            item = QTreeWidgetItem()
            item.setText(0, self.bin_to_hex(sub))
            item.setTextAlignment(0, 5)
            self.followers.addTopLevelItem(item)

        # Input for adding message to timeline
        self.edit = QLineEdit()
        self.edit.setPlaceholderText("Enter a command")

        # Button responsible for the sending of messages to the timelines of the respective followers
        self.sendButton = QPushButton("Post")

        # Button responsible for subscribing to other peers
        self.buttonSub = QPushButton("Subscribe")
        self.buttonSub.setObjectName("subButton")
        
        # Button responsible for unsubscribing other peers
        self.buttonUnsub = QPushButton("Unsubscribe")
        self.buttonUnsub.setObjectName("unsubButton")

        self.sendButton.setCursor(Qt.PointingHandCursor)
        self.buttonSub.setCursor(Qt.PointingHandCursor)
        self.buttonUnsub.setCursor(Qt.PointingHandCursor)

        # Full layout
        grid = QGridLayout()
        grid.addWidget(self.id_label, 0, 0, 1, 6)
        grid.addWidget(self.list, 1, 0, 1, 6)
        grid.addWidget(self.followers, 0, 6, 2, 2)
        grid.addWidget(self.edit, 2, 0, 1, 5)
        grid.addWidget(self.sendButton, 2, 5)
        grid.addWidget(self.buttonUnsub, 2, 7)
        grid.addWidget(self.buttonSub, 2, 6)


        # Set dialog layout
        self.setLayout(grid)

        # Add button signal to greetings slot
        self.sendButton.clicked.connect(self.send)

        self.buttonSub.clicked.connect(self.popupSub)
        self.buttonUnsub.clicked.connect(self.popupUnsub)
        
        # Auto Scroll when list scrollbar range is changed
        self.list.verticalScrollBar().rangeChanged.connect(self.resizeScrollFromList)

        # Auto Scroll when followers list scrollbar range is change
        self.followers.verticalScrollBar().rangeChanged.connect(self.resizeScrollFromFollowers)

        # When enter pressed send
        self.edit.setFocus()

        self.setWindowTitle('Timeline')

        # Threading
        self.sig = QtCore.Signal(tuple)
        self.t = Worker(self.socket_receiver)
        self.t.clear_signal.connect(self.clear_list)
        self.t.pull_signal.connect(self.create_post_item)
        self.t.start()

    def send(self):
        '''
            Sends a command
        '''

        item = QListWidgetItem()
        widget = QWidget()
        post = QLabel("<b><small>Placeholder: </small></b>" + self.edit.text())
        timeStamp = QLabel("<small>11:11</small>")
        layout = QHBoxLayout()
        layout.addWidget(post, alignment=Qt.AlignLeft)
        layout.addWidget(timeStamp, alignment=Qt.AlignRight)

        layout.setContentsMargins(0, 0, 0, 0)
        widget.setLayout(layout)
        # item.
        #self.list.addItem(item)
        #self.send_command(self.edit.text().replace('\n', ''))
        self.send_command(self.create_command("PUBLISH", self.edit.text().replace('\n', '')))
        self.list.setItemWidget(item, widget)
        self.edit.setText("")

    def resizeScrollFromList(self):
        self.list.verticalScrollBar().setValue(
            self.list.verticalScrollBar().maximum())
    
    def resizeScrollFromFollowers(self):
        self.followers.verticalScrollBar().setValue(
            self.followers.verticalScrollBar().maximum())

    def popupSub(self):
        popup = QDialog()
        popup.setObjectName('popup')
        popup.setModal(True)
        popup.setWindowTitle('Subscribe')
        
        # Title
        text = QLabel('Subscribe to a peer')

        # Input box
        sub_input = QLineEdit(parent=popup)
        sub_input.setPlaceholderText('Enter peer id')

        # Sub button
        sub_button = QPushButton("Subscribe")

        # Cancel button 
        cancel_button = QPushButton("Cancel")

        warning = QLabel()
        warning.setStyleSheet("background-color: #FFD2D2; color: black; padding: 0.2em; border-radius:15px;")
        warning.setAlignment(Qt.AlignCenter)

        # Layout
        hLayout = QHBoxLayout()
        hLayout.setSpacing(10)
        hLayout.addWidget(sub_button)
        hLayout.addWidget(cancel_button)

        vLayout = QVBoxLayout()
        vLayout.setSpacing(10)
        vLayout.addWidget(text, alignment=Qt.AlignCenter)
        vLayout.addWidget(sub_input)
        vLayout.addLayout(hLayout)
        vLayout.addWidget(warning)

        warning.setVisible(False)

        popup.setLayout(vLayout)

        cancel_button.clicked.connect(popup.close)

        def subscribe():
            it = QTreeWidgetItemIterator(self.followers)

            while it.value():
                if it.value().text(0) == sub_input.text():
                    warning.setVisible(True)
                    warning.setText("<b>Error:</b><br>" + "Already subbed to " + sub_input.text() + "!")
                    break
                it += 1

            if it.value() is None:
                self.send_command(self.create_command("SUBSCRIBE", self.hex_to_bin(sub_input.text())))
                item = QTreeWidgetItem()
                item.setText(0, sub_input.text())
                item.setTextAlignment(0, 5)
                self.followers.addTopLevelItem(item)
                list_item = QListWidgetItem()
                self.list.addItem(list_item)
                self.list.setItemWidget(list_item, QLabel("<small>Subscribed to " + sub_input.text() + "!</small>"))
                popup.close()

        sub_button.clicked.connect(subscribe)
        popup.exec()


    def popupUnsub(self):
        popup = QDialog()
        popup.setObjectName('popup')
        popup.setModal(True)
        popup.setWindowTitle('Unsubscribe')
        
        # Title
        text = QLabel('Unsubscribe to a peer')

        # Input box
        unsub_input = QLineEdit(parent=popup)
        unsub_input.setPlaceholderText('Enter peer id')

        # Sub button
        sub_button = QPushButton("Unsubscribe")

        # Cancel button 
        cancel_button = QPushButton("Cancel")

        # Layout
        hLayout = QHBoxLayout()
        hLayout.setSpacing(10)
        hLayout.addWidget(sub_button)
        hLayout.addWidget(cancel_button)

        warning = QLabel()
        warning.setStyleSheet("background-color: #FFD2D2; color: black; padding: 0.2em; border-radius:15px;")
        warning.setAlignment(Qt.AlignCenter)

        vLayout = QVBoxLayout()
        vLayout.setSpacing(10)
        vLayout.addWidget(text)
        vLayout.addWidget(unsub_input)
        vLayout.addLayout(hLayout)
        vLayout.addWidget(warning)

        warning.setVisible(False)
        popup.setLayout(vLayout)

        cancel_button.clicked.connect(popup.close)

        def unsubscribe():
            it = QTreeWidgetItemIterator(self.followers)
            value_removed = False

            i = 0
            while(it.value()):
                if it.value().text(0) == unsub_input.text():
                    self.followers.takeTopLevelItem(i)
                    value_removed = True
                    break
                it += 1
                i += 1

            if value_removed:
                self.send_command(self.create_command("UNSUBSCRIBE", self.hex_to_bin(unsub_input.text())))
                list_item = QListWidgetItem()
                self.list.addItem(list_item)
                self.list.setItemWidget(list_item, QLabel("<small>Unsubscribed to " + unsub_input.text() + "!</small>"))
                popup.close()
            else:
                warning.setVisible(True)
                warning.setText("<b>Error:</b><br>" + "Can't unsubscribe non followed peers!")


        sub_button.clicked.connect(unsubscribe)
        popup.exec()

    def clear_list(self):
        self.list.clear()

    def create_post_item(self, t):
        item = QListWidgetItem()
        widget = QWidget()
        placeholder = '<small style="color:white">' + t[2][:7] + ": </small>"
        if self.id == t[2]:
            placeholder = '<small style="color:black">' + t[2][:7] + ": </small>"
        post = QLabel('<b>' + placeholder + "</b>" + t[0])
        timestamp = QLabel("<small>"+t[1]+"</small>")
        layout = QHBoxLayout()
        layout.addWidget(post)
        layout.addWidget(timestamp, alignment=Qt.AlignRight)
        layout.setContentsMargins(0, 0, 0, 0)
        widget.setLayout(layout)
        self.list.addItem(item)
        self.list.setItemWidget(item, widget)

    def create_command(self, command="NONE", value=""):
        j = {"command": command, "value": value}
        return json.dumps(j)

    def bin_to_hex(self, bin):
        return hex(int(bin, 2))

    def hex_to_bin(self, hex):
        return bin(int(hex, 16))[2:]

    def send_command(self, message, response=False):
        self.mutex.acquire(True)
        data = b"{}"
        # Send
        self.socket_sender.send(len(message).to_bytes(2, byteorder="little"))
        self.socket_sender.send(bytes(message, 'utf-8'))
        # Receive
        if response:
            size = int.from_bytes(self.socket_sender.recv(2), "little")
            data = self.socket_sender.recv(size)
            if data.decode('utf-8') == "null":
                data = b"{}"
        self.mutex.release()
        return json.loads(data.decode('utf-8'))


    def start_socket(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            host = socket.gethostbyname("localhost")
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind((host, 7777))
            sock.listen()
            self.socket_sender, a = sock.accept()
            self.socket_receiver, a = sock.accept()

    def reject(self):
        self.t.is_running = False
        self.t.socket.shutdown(0)
        QDialog.reject(self)


dir_path = os.path.dirname(os.path.realpath(__file__))

if __name__ == '__main__':
    # Create the Qt Application
    app = QApplication(sys.argv)
    with open(dir_path + "/style.qss", "r") as f:
        _style = f.read()
        app.setStyleSheet(_style)
    # Create and show the form
    form = Form()
    form.showNormal()
    # Run the main Qt loop
    sys.exit(app.exec())
