from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app
from google.appengine.ext import db

import hashlib
import random

class Lobby(db.Model):
  lid = db.StringProperty(required=True, indexed=True)
  ctime = db.DateTimeProperty(auto_now_add=True, required=True) # creation time
  mtime = db.DateTimeProperty(auto_now=True, required=True) # last modification

class Peer(db.Model):
  pid = db.StringProperty(required=True, indexed=True)
  ip = db.StringProperty(required=True)
  port = db.IntegerProperty(required=True)
  ctime = db.DateTimeProperty(auto_now_add=True, required=True) # creation time
  mtime = db.DateTimeProperty(auto_now=True, required=True) # last modification

class ActivePeers(db.Model):
  lid = db.StringProperty(required=True, indexed=True)
  pid = db.StringProperty(required=True)


class NewUser(webapp.RequestHandler):
  def get(self):
    self.handle()
  def post(self):
    self.handle()
  def handle(self):
    port = int(self.request.get('o', default_value='3938'))
    if port is 0: port = 3938
    pid = hashlib.sha224(self.request.url + self.request.remote_addr
                       + str(random.random())).hexdigest()
    p = Peer(pid=pid, ip=self.request.remote_addr, port=port)
    p.put()

    self.response.out.write("<?xml version=\"1.0\"?>\n")
    self.response.out.write("<eyeunite>\n")
    self.response.out.write("  <peer>\n")
    self.response.out.write("    <pid>" + p.pid + "</pid>\n")
    self.response.out.write("    <ip>" + p.ip + "</ip>\n")
    self.response.out.write("    <port>" + str(p.port) + "</port>\n")
    self.response.out.write("  </peer>\n")
    self.response.out.write("</eyeunite>\n")


class NewLobby(webapp.RequestHandler):
  def get(self):
    self.handle()
  def post(self):
    self.handle()
  def handle(self):
    port = int(self.request.get('o', default_value='3938'))
    if port is 0: port = 3938
    pid = self.request.get('p', default_value='')
    if pid is '':
      pid = hashlib.sha224(self.request.url + self.request.remote_addr
                         + str(random.random())).hexdigest()
      p = Peer(pid=pid, ip=self.request.remote_addr, port=port)
      p.put()
    else:
      p = db.GqlQuery("SELECT * FROM Peer WHERE pid = :1", pid).get()
      if p is None:
        self.response.out.write("Error: invalid pid")
        return

    lid = hashlib.sha224(self.request.url + self.request.remote_addr
                       + str(random.random())).hexdigest()
    l = Lobby(lid=lid)
    l.put()

    ap = ActivePeers(lid=l.lid, pid=p.pid)
    ap.put()

    self.response.out.write("<?xml version=\"1.0\"?>\n")
    self.response.out.write("<eyeunite>\n")
    self.response.out.write("  <lid>" + l.lid + "</lid>\n")
    self.response.out.write("  <peer>\n")
    self.response.out.write("    <pid>" + p.pid + "</pid>\n")
    self.response.out.write("    <ip>" + p.ip + "</ip>\n")
    self.response.out.write("    <port>" + str(p.port) + "</port>\n")
    self.response.out.write("  </peer>\n")
    self.response.out.write("</eyeunite>\n")


class JoinLobby(webapp.RequestHandler):
  def get(self):
    self.handle()
  def post(self):
    self.handle()
  def handle(self):
    lid = self.request.get('l', default_value='')
    if lid is '':
      self.response.out.write("Error: no lid")
      return

    l = db.GqlQuery("SELECT * FROM Lobby WHERE lid = :1", lid).get()
    if l is None:
      self.response.out.write("Error: invalid lid")
      return

    p = None
    port = int(self.request.get('o', default_value='3938'))
    if port is 0: port = 3938
    pid = self.request.get('p', default_value='')
    if pid is '':
      pid = hashlib.sha224(self.request.url + self.request.remote_addr
                         + str(random.random())).hexdigest()
      p = Peer(pid=pid, ip=self.request.remote_addr, port=port)
      p.put()
    else:
      p = db.GqlQuery("SELECT * FROM Peer WHERE pid = :1", pid).get()
      if p is None:
        self.response.out.write("Error: invalid pid")
        return

    self.response.out.write("<?xml version=\"1.0\"?>\n")
    self.response.out.write("<eyeunite>\n")
    self.response.out.write("  <lid>" + l.lid + "</lid>\n")
    self.response.out.write("  <peer>\n")
    self.response.out.write("    <pid>" + p.pid + "</pid>\n")
    self.response.out.write("    <ip>" + p.ip + "</ip>\n")
    self.response.out.write("    <port>" + str(p.port) + "</port>\n")
    self.response.out.write("  </peer>\n")
    self.response.out.write("</eyeunite>\n")


class ListLobby(webapp.RequestHandler):
  def get(self):
    self.handle()
  def post(self):
    self.handle()
  def handle(self):
    lid = self.request.get('l', default_value='')
    if lid is '':
      self.response.out.write("Error: no lid")
      return

    self.response.out.write("<?xml version=\"1.0\"?>\n")
    self.response.out.write("<eyeunite>\n")
    self.response.out.write("  <lid>" + l.lid + "</lid>\n")

    activepeers = db.GqlQuery("SELECT * FROM ActivePeers WHERE lid = :1", lid)
    for ap in activepeers:
      # look up p's IP
      p = db.GqlQuery("SELECT * FROM Peer WHERE pid = :1", ap.pid).get()
      if p is None:
        self.response.out.write("Error: invalid pid")
        return
      self.response.out.write("  <peer>\n")
      self.response.out.write("    <pid>" + p.pid + "</pid>\n")
      self.response.out.write("    <ip>" + p.ip + "</ip>\n")
      self.response.out.write("    <port>" + str(p.port) + "</port>\n")
      self.response.out.write("  </peer>\n")
    self.response.out.write("</eyeunite>\n")


application = webapp.WSGIApplication(
  [ ('/u', NewUser),
    ('/n', NewLobby),
    ('/j', JoinLobby),
    ('/l', ListLobby) ],
  debug = True)

def main():
  run_wsgi_app(application)

if __name__ == "__main__":
  main()
