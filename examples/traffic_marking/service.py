from flask import Flask
from flask import request
import os
import requests
import socket
import sys
import MySQLdb

app = Flask(__name__)

TRACE_HEADERS_TO_PROPAGATE = [
    'X-Ot-Span-Context',
    'X-Request-Id',

    # Zipkin headers
    'X-B3-TraceId',
    'X-B3-SpanId',
    'X-B3-ParentSpanId',
    'X-B3-Sampled',
    'X-B3-Flags',

    # Jaeger header (for native client)
    "uber-trace-id"
]


@app.route('/service/<service_number>')
def hello(service_number):
  return ('Hello from behind Envoy (service {})! hostname: {} resolved'
          'hostname: {}\n'.format(os.environ['SERVICE_NAME'], socket.gethostname(),
                                  socket.gethostbyname(socket.gethostname())))


@app.route('/person_info/<id>')
def get_person_info(id):
  db = MySQLdb.connect("mysql-envoy-proxy", "root", "root123", "person", charset='utf8',port=13306 )
  cursor = db.cursor()
  sql = "SELECT * FROM person WHERE id = %u" % (id)
  try:
    cursor.execute(sql)
    results = cursor.fetchall()
    for row in results:
      fname = row[0]
      lname = row[1]
      age = row[2]
      sex = row[3]
      income = row[4]
      return ('Person <{}> information: firstName: {}, lastName: {}, age: {},'
                  'sex: {}, income: {}\n'.format(id,fname,lname,age,sex,income))
  except:
     return ("Error: unable to fetch data")



if __name__ == "__main__":
  app.run(host='127.0.0.1', port=9080, debug=True)
