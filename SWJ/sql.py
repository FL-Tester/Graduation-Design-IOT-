import time
import pymysql
from paho.mqtt import client as mqtt_client
import json

def update_db( db):
    cursor = db.cursor()
    sql = "INSERT INTO %s (node, temp, humi, light, wp, soilhumi ,time)VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')" % ("FL_GP", 1, 1, 1, 1, 1, 1, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
    cursor.execute(sql)
    db.commit()

if __name__ == '__main__': 
    db = pymysql.connect(host='175.178.79.144',
                     user='root',
                     password='dnFjfpw77zYyNA6Z',
                     database='cd_db')
    
    cursor = db.cursor()
    sql = "INSERT INTO %s (node, temp, humi, light, wp, soilhumi ,time)VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')" % ("FL_GP", 1, 1, 1, 1, 1, 1, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
    cursor.execute(sql)
    # #删除表格所有数据
    # sql = "DELETE FROM %s" % ("FL_GP")
    # cursor.execute(sql)
    # ## id 重置
    # sql = "ALTER TABLE %s AUTO_INCREMENT = 1" % ("FL_GP")
    # cursor.execute(sql)
    
    db.commit()
    db.close()

