import requests
import rospy
from abc import ABC
from enum import Enum


class MicroServiceError(Exception):

    class Code(Enum):
        UNDEFINED = 0
        CONNECTION_ERROR = 1
        BAD_STATUS_CODE = 2
        BAD_REQUEST_CONTENT = 3
        BAD_RESPONSE_CONTENT = 4

    def __init__(self, message, code=Code.UNDEFINED):
        self.code = code
        super().__init__(message)


class ABCMicroService(ABC):
    API_PREFIX = 'api/v1'
    MICROSERVICE_URI = ''

    def __init__(self, base_url, headers):
        self._base_url = f'{base_url}/{self.MICROSERVICE_URI}/{self.API_PREFIX}'
        self._headers = headers

    def update_header(self, key, value):
        self._headers[key] = value


class ABCReportMicroService(ABCMicroService):
    RESOURCE_URI = ''

    def __init__(self, base_url, header):
        super().__init__(base_url, header)

    def ping(self):
        endpoint = f'{self._base_url}/ping'
        try:
            response = requests.get(endpoint, headers=self._headers)
        except requests.ConnectionError as e:
            rospy.logdebug(e)
            return False
        rospy.logdebug('Cloud API responded with code: {}'.format(response.status_code))
        return response.status_code == 200

    def send(self, payload):
        endpoint = f'{self._base_url}/{self.MICROSERVICE_URI}'
        try:
            rospy.loginfo(f'url: {endpoint}, headers: {self._headers}, payload: {payload}')
            response = requests.post(endpoint, headers=self._headers, json=payload)
        except requests.ConnectionError as connection_error:
            raise MicroServiceError(str(connection_error), code=MicroServiceError.Code.CONNECTION_ERROR)
        rospy.logdebug('Cloud API responded with code: {}'.format(response.status_code))
        if response.status_code != 200:
            raise MicroServiceError(
                (f'{self._base_url}: {self.__class__.__name__} '
                 f'responded with status {response.status_code}: {response.text}'),
                code=MicroServiceError.Code.BAD_STATUS_CODE,
            )


class AuthentificationMS(ABCMicroService):
    MICROSERVICE_URI = 'authentification'

    def call(self):
        url = f'{self._base_url}/token/{self._headers["identifier"]}'
        try:
            response = requests.get(url, headers=self._headers)
        except requests.ConnectionError as connection_error:
            raise MicroServiceError(str(connection_error), code=MicroServiceError.Code.CONNECTION_ERROR)
        rospy.logdebug('Cloud API responded with code: {}'.format(response.status_code))

        if response.status_code == 404:
            raise MicroServiceError(f'There is no robot registered with the identifier "{self._headers["identifier"]}',
                                    code=MicroServiceError.Code.BAD_REQUEST_CONTENT)

        json = response.json()
        if response.status_code != 200 or response.status_code == 200 and json['error'] is True:
            raise MicroServiceError(
                f'{url}: {self.__class__.__name__} responded with status {response.status_code}: {response.text}',
                code=MicroServiceError.Code.BAD_STATUS_CODE)

        if 'data' not in json or 'apikey' not in json['data']:
            raise MicroServiceError(f'{url}: {self.__class__.__name__}: invalid data payload: {json}',
                                    code=MicroServiceError.Code.BAD_RESPONSE_CONTENT)
        return json['data']['apikey']


class DailyReportMS(ABCReportMicroService):
    MICROSERVICE_URI = 'daily-reports'


class TestReportMS(ABCReportMicroService):
    MICROSERVICE_URI = 'test-reports'


class AlertReportMS(ABCReportMicroService):
    MICROSERVICE_URI = 'alert-reports'


class AutoDiagnosisReportMS(ABCReportMicroService):
    MICROSERVICE_URI = 'auto-diagnosis-reports'


class FakeReportMS(ABCReportMicroService):

    def send(self, payload):
        return True


class CloudAPI(object):

    def __init__(self, domain, serial_number, rasp_id, api_key, sharing_allowed, https=False):
        self.__base_url = '{}://{}'.format('https' if https else 'http', domain)
        self.__url = self.__base_url
        self.__sharing_allowed = sharing_allowed
        if not serial_number and not rasp_id:
            raise ValueError('There must be at least a serial number or a rasp id')
        self.__headers = {
            'accept': 'application/json',
            'identifier': rasp_id,
            'apiKey': api_key,
        }
        if serial_number != '':
            self.__headers['identifier'] = serial_number
        self.__microservices = {}
        self.__init_microservices()

    def __init_microservices(self):
        self.__microservices = {'authentification': AuthentificationMS(self.__base_url, self.__headers)}
        if self.__sharing_allowed:
            self.__microservices.update({
                'daily_reports': DailyReportMS(self.__base_url, self.__headers),
                'test_reports': TestReportMS(self.__base_url, self.__headers),
                'alert_reports': AlertReportMS(self.__base_url, self.__headers),
                'auto_diagnosis_reports': AutoDiagnosisReportMS(self.__base_url, self.__headers)
            })
        else:
            self.__microservices.update({
                'daily_reports': FakeReportMS(self.__base_url, self.__headers),
                'test_reports': FakeReportMS(self.__base_url, self.__headers),
                'alert_reports': FakeReportMS(self.__base_url, self.__headers),
                'auto_diagnosis_reports': FakeReportMS(self.__base_url, self.__headers)
            })

    def set_identifier(self, value):
        for microservice in self.__microservices.values():
            microservice.update_header('identifier', value)

    def set_api_key(self, value):
        for microservice in self.__microservices.values():
            microservice.update_header('apiKey', value)

    def set_sharing_allowed(self, value):
        self.__sharing_allowed = value
        self.__init_microservices()

    @property
    def authentification(self):
        return self.__microservices['authentification']

    @property
    def daily_reports(self):
        return self.__microservices['daily_reports']

    @property
    def test_reports(self):
        return self.__microservices['test_reports']

    @property
    def alert_reports(self):
        return self.__microservices['alert_reports']

    @property
    def auto_diagnosis_reports(self):
        return self.__microservices['auto_diagnosis_reports']
