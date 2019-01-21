from PyQt5.QtWidgets import QWidget, QLabel, QGridLayout
from PyQt5.QtCore import Qt, pyqtSlot
import numpy as np
from PyQt5.QtGui import QPixmap, QImage

import os

class MainWindow(QWidget):
	name = "MainWindow"
	
	def __init__(self):
		super().__init__()
		self.init_ui()
	
	def init_ui(self):
		self.setWindowTitle(os.path.basename(__file__))
		self.move(100, 100)
		
		# Labels / Image containers
		self.label_img_blur = QLabel()
		self.label_img_blur.setText("Blur")
		self.img_blur = QLabel()
		
		self.label_img_gamma = QLabel()
		self.label_img_gamma.setText("Gamma")
		self.img_gamma = QLabel()
		
		self.label_img_threshold = QLabel()
		self.label_img_threshold.setText("Threshold")
		self.img_threshold = QLabel()
		
		self.label_img_scores = QLabel()
		self.label_img_scores.setText("Scores")
		self.img_scores = QLabel()
		
		self.label_img_result = QLabel()
		self.label_img_result.setText("Result")
		self.img_result = QLabel()
		
		# Grid layout
		widget_layout = QGridLayout()
		widget_layout.addWidget(self.label_img_blur, 0, 0, Qt.AlignLeft)
		widget_layout.addWidget(self.img_blur, 1, 0, Qt.AlignLeft)
		
		widget_layout.addWidget(self.label_img_gamma, 0, 1, Qt.AlignLeft)
		widget_layout.addWidget(self.img_gamma, 1, 1, Qt.AlignLeft)
		
		widget_layout.addWidget(self.label_img_threshold, 0, 2, Qt.AlignLeft)
		widget_layout.addWidget(self.img_threshold, 1, 2, Qt.AlignLeft)
		
		widget_layout.addWidget(self.label_img_scores, 0, 3, Qt.AlignLeft)
		widget_layout.addWidget(self.img_scores, 1, 3, Qt.AlignLeft)
		
		widget_layout.addWidget(self.label_img_result, 0, 4, Qt.AlignLeft)
		widget_layout.addWidget(self.img_result, 1, 4, Qt.AlignLeft)
		self.setLayout(widget_layout)
		
	@pyqtSlot(np.ndarray)
	def update_blur(self, image=None):
		image = image.copy()
		height, width = image.shape
		qImage = QImage(image.data, width, height, width, QImage.Format_Indexed8)
		self.img_blur.setPixmap(QPixmap(qImage))
		self.img_blur.show()	
		
	@pyqtSlot(np.ndarray)
	def update_gamma(self, image=None):
		image = image.copy()
		height, width = image.shape
		qImage = QImage(image.data, width, height, width, QImage.Format_Indexed8)
		self.img_gamma.setPixmap(QPixmap(qImage))
		self.img_gamma.show()	

	@pyqtSlot(np.ndarray)
	def update_threshold(self, image=None):
		image = image.copy()
		height, width = image.shape
		qImage = QImage(image.data, width, height, width, QImage.Format_Indexed8)
		self.img_threshold.setPixmap(QPixmap(qImage))
		self.img_threshold.show()	
		
	@pyqtSlot(np.ndarray)
	def update_scores(self, image=None):
		image = image.copy()
		height, width = image.shape[:2]
		qImage = QImage(image.data, width, height, width * 3, QImage.Format_RGB888)
		self.img_scores.setPixmap(QPixmap(qImage))
		self.img_scores.show()	
		
	@pyqtSlot(np.ndarray)
	def update_result(self, image=None):
		image = image.copy()
		height, width = image.shape
		qImage = QImage(image.data, width, height, width, QImage.Format_Indexed8)
		self.img_result.setPixmap(QPixmap(qImage))
		self.img_result.show()	
