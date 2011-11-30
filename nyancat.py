#!/usr/bin/env python
import SocketServer
from subprocess import Popen, PIPE
from telnetsrvlib import TelnetHandler

class TNS(SocketServer.TCPServer):
	allow_reuse_address = True

class TNH(TelnetHandler):
	def handle(self):
		print self.TERM
		p = Popen(["./nyancat"], shell=False, stdout=PIPE, stdin=PIPE, bufsize=0)
		if (self.TERM.lower().find("xterm") != -1):
			p.stdin.write("1\n")
		elif (self.TERM.lower().find("linux") != -1):
			p.stdin.write("3\n")
		elif (self.TERM.lower().find("fallback") != -1):
			p.stdin.write("4\n")
		else:
			p.stdin.write("2\n")
		while 1:
			s = p.stdout.read(1024)
			try:
				self.write(s)
			except:
				p.kill()
				return

tns = TNS(("0.0.0.0", 23), TNH)
tns.serve_forever()
