import json
import os


class ReportHandler:
    def __init__(self, path):
        self.set_path(path)

    def set_path(self, path):
        self.__path = path
        if not os.path.isfile(self.__path):
            open(self.__path, 'w+').close()
        self.__read()

    def __read(self):
        with open(self.__path, 'r') as f:
            try:
                self.content = json.load(f)
            except ValueError:
                self.content = {'logs': []}

    def __write(self):
        with open(self.__path, 'w') as f:
            json.dump(self.content, f)

    def add_log(self, msg, log_type, date):
        inserted = False
        for index, log in enumerate(self.content['logs']):
            if log['msg'] == msg:
                self.content['logs'][index]['occurred'].append(date)
                inserted = True
                break
        if not inserted:
            self.content['logs'].append({
                'msg': msg,
                'type': log_type,
                'occurred': [date]
            })
        self.__write()
