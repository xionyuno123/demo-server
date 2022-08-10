import threading
import time
import pymongo

bits = 123
pkts = 214
drop = 412

class ThreadWorker(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
    
    def run(self):
        mongo_client = pymongo.MongoClient('127.0.0.1', 27017)
        mongo_db = mongo_client['netstats-db']
        mongo_collection = mongo_db['netstats']

        find_condition = {
            'id' : 'shared',
        }
        find_result_cursor = mongo_collection.find(find_condition)
        count = 0
        for find_result in find_result_cursor:
            print(find_result)
            count += 1
        if count != 1:
            print("invalid database format")
            return

        while True:
            time.sleep(0.01)
            info = {
                "id": "shared",
                "bits_received": bits,
                "pkts_received": pkts,
                "pkts_missed": drop,
            }
            update_condition = {'id' : 'shared'}
            mongo_collection.update_many(update_condition, {'$set' : info}, upsert= True)


thread1 = ThreadWorker()
thread1.start()
thread1.join()
print("end")