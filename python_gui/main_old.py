import sys
import re
import json
from collections import deque
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QTextEdit, QFrame, QLineEdit,
    QTabWidget, QFileDialog, QMessageBox
)
from PySide6.QtCore import Qt, QDateTime
from PySide6.QtGui import QFont, QPainter, QPen, QColor, QPolygonF
from PySide6.QtCore import QPointF
from serial_worker import SerialWorker, list_ports

BAUDRATES = ["9600", "19200", "38400", "57600", "115200", "230400"]
MAX_BUFFERS = 3  # Cuántos JSONs acumula el gráfico

STYLE = """
QMainWindow, QWidget {
    background-color: #f0f0f0;
    color: #000000;
    font-family: 'Segoe UI', sans-serif;
    font-size: 13px;
}
QTabWidget::pane {
    border: 1px solid #adadad;
    background-color: #f0f0f0;
}
QTabBar::tab {
    background-color: #e1e1e1;
    border: 1px solid #adadad;
    border-bottom: none;
    padding: 4px 14px;
    color: #000000;
}
QTabBar::tab:selected {
    background-color: #f0f0f0;
    border-bottom: 1px solid #f0f0f0;
}
QTabBar::tab:hover { background-color: #e5f1fb; }
QComboBox {
    background-color: #ffffff;
    border: 1px solid #7a7a7a;
    border-radius: 0px;
    padding: 3px 8px;
    color: #000000;
    min-height: 22px;
}
QComboBox::drop-down { border-left: 1px solid #7a7a7a; width: 18px; }
QComboBox QAbstractItemView {
    background-color: #ffffff;
    selection-background-color: #0078d7;
    selection-color: #ffffff;
    color: #000000;
}
QPushButton {
    background-color: #e1e1e1;
    border: 1px solid #adadad;
    border-radius: 0px;
    padding: 4px 12px;
    color: #000000;
    min-height: 23px;
}
QPushButton:hover { background-color: #e5f1fb; border-color: #0078d7; }
QPushButton:pressed { background-color: #cce4f7; border-color: #005499; }
QPushButton:disabled { color: #a0a0a0; border-color: #cccccc; background-color: #f0f0f0; }
QPushButton#btn_connect {
    background-color: #e1e1e1;
    border-color: #adadad;
    color: #006400;
    font-weight: bold;
}
QPushButton#btn_connect:hover { background-color: #e5f1fb; border-color: #0078d7; }
QPushButton#btn_connect:disabled { color: #a0a0a0; }
QPushButton#btn_disconnect {
    background-color: #e1e1e1;
    border-color: #adadad;
    color: #a00000;
    font-weight: bold;
}
QPushButton#btn_disconnect:hover { background-color: #e5f1fb; border-color: #0078d7; }
QPushButton#btn_send {
    background-color: #0078d7;
    border-color: #005499;
    color: #ffffff;
    font-weight: bold;
}
QPushButton#btn_send:hover { background-color: #006cc1; }
QPushButton#btn_send:pressed { background-color: #005499; }
QPushButton#btn_send:disabled { background-color: #cccccc; border-color: #bbbbbb; color: #888888; }
QLineEdit {
    background-color: #ffffff;
    border: 1px solid #7a7a7a;
    border-radius: 0px;
    padding: 3px 8px;
    color: #000000;
    min-height: 22px;
}
QLineEdit:focus { border-color: #0078d7; }
QLineEdit:disabled { background-color: #f0f0f0; color: #a0a0a0; }
QTextEdit {
    background-color: #ffffff;
    border: 1px solid #7a7a7a;
    border-radius: 0px;
    color: #0000aa;
    font-family: 'Consolas', monospace;
    font-size: 12px;
    padding: 4px;
}
QLabel#section_label {
    color: #444444;
    font-size: 11px;
    letter-spacing: 0px;
    font-weight: bold;
}
QFrame#separator {
    background-color: #adadad;
    max-height: 1px;
}
"""

# Colores para cada buffer en el gráfico
BUFFER_COLORS = ["#0078d7", "#d74200", "#008000"]


class SignalPlot(QWidget):
    """Widget que dibuja la señal acumulada de hasta MAX_BUFFERS JSONs."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(200)
        self.buffers = deque(maxlen=MAX_BUFFERS)  # cada elemento: lista de muestras
        self.setStyleSheet("background-color: #ffffff; border: 1px solid #7a7a7a;")

    def add_buffer(self, samples: list):
        self.buffers.append(samples)
        self.update()  # repintar

    def clear_plot(self):
        self.buffers.clear()
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        w = self.width()
        h = self.height()
        pad = 30  # margen interno

        # Fondo blanco
        painter.fillRect(self.rect(), QColor("#ffffff"))

        if not self.buffers:
            painter.setPen(QColor("#aaaaaa"))
            painter.drawText(self.rect(), Qt.AlignCenter, "Sin datos")
            return

        # Calcular todos los valores para escala global
        all_samples = [s for buf in self.buffers for s in buf]
        y_min = min(all_samples)
        y_max = max(all_samples)
        if y_max == y_min:
            y_max = y_min + 1

        total_samples = sum(len(b) for b in self.buffers)
        plot_w = w - 2 * pad
        plot_h = h - 2 * pad

        # Ejes
        axis_pen = QPen(QColor("#aaaaaa"), 1)
        painter.setPen(axis_pen)
        painter.drawLine(pad, pad, pad, h - pad)           # eje Y
        painter.drawLine(pad, h - pad, w - pad, h - pad)  # eje X

        # Etiquetas Y
        painter.setPen(QColor("#555555"))
        painter.setFont(QFont("Consolas", 8))
        painter.drawText(2, pad + 4, str(int(y_max)))
        painter.drawText(2, h - pad, str(int(y_min)))

        # Dibujar cada buffer con su color
        x_offset = 0
        for idx, buf in enumerate(self.buffers):
            color = QColor(BUFFER_COLORS[idx % len(BUFFER_COLORS)])
            pen = QPen(color, 1.5)
            painter.setPen(pen)

            n = len(buf)
            points = QPolygonF()
            for i, val in enumerate(buf):
                x = pad + (x_offset + i) * plot_w / (total_samples - 1) if total_samples > 1 else pad
                y = (h - pad) - (val - y_min) / (y_max - y_min) * plot_h
                points.append(QPointF(x, y))

            painter.drawPolyline(points)

            # Separador entre buffers (línea vertical punteada)
            if idx < len(self.buffers) - 1:
                sep_x = pad + (x_offset + n - 1) * plot_w / (total_samples - 1) if total_samples > 1 else pad
                sep_pen = QPen(QColor("#cccccc"), 1, Qt.DashLine)
                painter.setPen(sep_pen)
                painter.drawLine(int(sep_x), pad, int(sep_x), h - pad)

            x_offset += n

        # Leyenda
        painter.setFont(QFont("Segoe UI", 8))
        for idx, buf in enumerate(self.buffers):
            color = QColor(BUFFER_COLORS[idx % len(BUFFER_COLORS)])
            painter.setPen(QPen(color, 2))
            lx = pad + idx * 80
            painter.drawLine(lx, 12, lx + 16, 12)
            painter.setPen(QColor("#333333"))
            painter.drawText(lx + 20, 16, f"Buffer {idx + 1} ({len(buf)} muestras)")


def parse_json_buffer(raw: str):
    """
    Parsea el formato custom del STM32:
    { "ts":00:13:29 , "Muestras": 6025, 6366, ... }
    Devuelve lista de ints o None si falla.
    """
    try:
        # Extraer la parte de muestras
        m = re.search(r'"Muestras"\s*:\s*([\d,\s]+)', raw)
        if not m:
            return None
        nums_str = m.group(1)
        samples = [int(x.strip()) for x in nums_str.split(',') if x.strip()]
        return samples if samples else None
    except Exception:
        return None


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("STM32 Monitor")
        self.setMinimumSize(680, 580)
        self.worker = SerialWorker()
        self.worker.data_received.connect(self.on_data_received)
        self.worker.connection_error.connect(self.on_error)
        self._build_ui()
        self.setStyleSheet(STYLE)
        self._set_connected(False)

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(16, 16, 16, 16)
        root.setSpacing(12)

        # --- Conexión ---
        root.addWidget(self._label("CONEXIÓN SERIE"))
        conn_row = QHBoxLayout()
        conn_row.setSpacing(8)

        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(100)
        self._refresh_ports()

        self.baud_combo = QComboBox()
        self.baud_combo.addItems(BAUDRATES)
        self.baud_combo.setCurrentText("115200")
        self.baud_combo.setFixedWidth(100)

        self.btn_refresh = QPushButton("↺")
        self.btn_refresh.setFixedWidth(36)
        self.btn_refresh.setToolTip("Refrescar puertos")
        self.btn_refresh.clicked.connect(self._refresh_ports)

        self.btn_connect = QPushButton("Conectar")
        self.btn_connect.setObjectName("btn_connect")
        self.btn_connect.clicked.connect(self._connect)

        self.btn_disconnect = QPushButton("Desconectar")
        self.btn_disconnect.setObjectName("btn_disconnect")
        self.btn_disconnect.clicked.connect(self._disconnect)

        conn_row.addWidget(self.port_combo, 1)
        conn_row.addWidget(self.baud_combo)
        conn_row.addWidget(self.btn_refresh)
        conn_row.addWidget(self.btn_connect)
        conn_row.addWidget(self.btn_disconnect)
        root.addLayout(conn_row)

        self.status_label = QLabel()
        self.status_label.setAlignment(Qt.AlignLeft)
        root.addWidget(self.status_label)

        root.addWidget(self._separator())

        # --- Enviar ---
        root.addWidget(self._label("ENVIAR DATOS"))
        send_row = QHBoxLayout()
        send_row.setSpacing(8)

        self.input_field = QLineEdit()
        self.input_field.setPlaceholderText("Escribí el dato a enviar...")
        self.input_field.returnPressed.connect(self._send_input)

        self.btn_send = QPushButton("Enviar")
        self.btn_send.setObjectName("btn_send")
        self.btn_send.setFixedWidth(80)
        self.btn_send.clicked.connect(self._send_input)

        send_row.addWidget(self.input_field, 1)
        send_row.addWidget(self.btn_send)
        root.addLayout(send_row)

        root.addWidget(self._separator())

        # --- Tabs: Log / Gráfico ---
        self.tabs = QTabWidget()

        # Tab Log
        log_widget = QWidget()
        log_layout = QVBoxLayout(log_widget)
        log_layout.setContentsMargins(0, 8, 0, 0)
        log_layout.setSpacing(6)

        log_header = QHBoxLayout()
        log_header.addWidget(self._label("LOG"))
        log_header.addStretch()

        btn_clear = QPushButton("Limpiar")
        btn_clear.setFixedWidth(70)
        btn_clear.clicked.connect(self._clear_log)

        btn_export = QPushButton("Exportar .txt")
        btn_export.setFixedWidth(90)
        btn_export.clicked.connect(self._export_log)

        log_header.addWidget(btn_clear)
        log_header.addWidget(btn_export)
        log_layout.addLayout(log_header)

        self.log = QTextEdit()
        self.log.setReadOnly(True)
        log_layout.addWidget(self.log, 1)

        self.tabs.addTab(log_widget, "Log")

        # Tab Gráfico
        plot_widget = QWidget()
        plot_layout = QVBoxLayout(plot_widget)
        plot_layout.setContentsMargins(0, 8, 0, 0)
        plot_layout.setSpacing(6)

        plot_header = QHBoxLayout()
        plot_header.addWidget(self._label(f"SEÑAL  (últimos {MAX_BUFFERS} buffers)"))
        plot_header.addStretch()

        btn_clear_plot = QPushButton("Limpiar gráfico")
        btn_clear_plot.setFixedWidth(100)
        btn_clear_plot.clicked.connect(self._clear_plot)
        plot_header.addWidget(btn_clear_plot)
        plot_layout.addLayout(plot_header)

        self.plot = SignalPlot()
        plot_layout.addWidget(self.plot, 1)

        self.tabs.addTab(plot_widget, "Gráfico")

        root.addWidget(self.tabs, 1)

    # ------------------------------------------------------------------ helpers

    def _label(self, text):
        lbl = QLabel(text)
        lbl.setObjectName("section_label")
        return lbl

    def _separator(self):
        sep = QFrame()
        sep.setObjectName("separator")
        sep.setFrameShape(QFrame.HLine)
        return sep

    # ------------------------------------------------------------------ puertos

    def _refresh_ports(self):
        self.port_combo.clear()
        ports = list_ports()
        if ports:
            self.port_combo.addItems(ports)
        else:
            self.port_combo.addItem("Sin puertos")

    # ------------------------------------------------------------------ conexión

    def _connect(self):
        port = self.port_combo.currentText()
        baud = int(self.baud_combo.currentText())
        ok = self.worker.connect(port, baud)
        if ok:
            self._set_connected(True)
            self._log_system(f"Conectado en {port} @ {baud} baud")

    def _disconnect(self):
        self.worker.disconnect()
        self._set_connected(False)
        self._log_system("Desconectado")

    def _set_connected(self, connected: bool):
        self.btn_connect.setEnabled(not connected)
        self.btn_disconnect.setEnabled(connected)
        self.port_combo.setEnabled(not connected)
        self.baud_combo.setEnabled(not connected)
        self.btn_refresh.setEnabled(not connected)
        self.btn_send.setEnabled(connected)
        self.input_field.setEnabled(connected)
        if connected:
            self.status_label.setText("● Conectado")
            self.status_label.setStyleSheet("color: #4cda85; font-size: 12px;")
        else:
            self.status_label.setText("○ Desconectado")
            self.status_label.setStyleSheet("color: #666; font-size: 12px;")

    # ------------------------------------------------------------------ enviar

    def _send_input(self):
        text = self.input_field.text()
        if text:
            self._send(text)
            self.input_field.clear()

    def _send(self, value: str):
        self.worker.send(value)
        self._log_tx(value)

    # ------------------------------------------------------------------ datos RX

    def on_data_received(self, data: str):
        self._log_rx(data)

        # Intentar parsear como buffer de señal
        samples = parse_json_buffer(data)
        if samples:
            self.plot.add_buffer(samples)

    def on_error(self, msg: str):
        self._log_system(f"Error: {msg}")
        self._set_connected(False)

    # ------------------------------------------------------------------ log

    def _log_rx(self, text):
        ts = QDateTime.currentDateTime().toString("hh:mm:ss")
        self.log.append(f'<span style="color:#555">[{ts}]</span> <span style="color:#4cda85">RX</span> {text}')

    def _log_tx(self, text):
        ts = QDateTime.currentDateTime().toString("hh:mm:ss")
        self.log.append(f'<span style="color:#555">[{ts}]</span> <span style="color:#4ca8da">TX</span> {text}')

    def _log_system(self, text):
        ts = QDateTime.currentDateTime().toString("hh:mm:ss")
        self.log.append(f'<span style="color:#555">[{ts}] {text}</span>')

    def _clear_log(self):
        self.log.clear()

    def _export_log(self):
        path, _ = QFileDialog.getSaveFileName(
            self, "Exportar log", "", "Archivos de texto (*.txt)"
        )
        if path:
            try:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(self.log.toPlainText())
            except Exception as e:
                QMessageBox.critical(self, "Error", f"No se pudo guardar:\n{e}")

    # ------------------------------------------------------------------ gráfico

    def _clear_plot(self):
        self.plot.clear_plot()

    # ------------------------------------------------------------------ cierre

    def closeEvent(self, event):
        self.worker.disconnect()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
