from datetime import datetime, timedelta, tzinfo
import time as _time
import unittest
ZERO = timedelta(0);

class UTC(tzinfo):
    """UTC"""

    def utcoffset(self, dt):
        return ZERO

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return ZERO

class LocalTimezone(tzinfo):

	def __init__(self):
		self.STDOFFSET = timedelta(seconds = -_time.timezone)
		if _time.daylight:
			self.DSTOFFSET = timedelta(seconds = -_time.altzone)
		else:
			self.DSTOFFSET = self.STDOFFSET

		self.DSTDIFF = self.DSTOFFSET - self.STDOFFSET
	def utcoffset(self, dt):
		if self._isdst(dt):
			return self.DSTOFFSET
		else:
			return self.STDOFFSET

	def dst(self, dt):
		if self._isdst(dt):
			return self.DSTDIFF
		else:
			return ZERO

	def tzname(self, dt):
		return _time.tzname[self._isdst(dt)]

	def _isdst(self, dt):
		tt = (dt.year, dt.month, dt.day,
			dt.hour, dt.minute, dt.second,
			dt.weekday(), 0, -1)
		stamp = _time.mktime(tt)
		tt = _time.localtime(stamp)
		return tt.tm_isdst > 0

def now2fos():
        '''Return time in seconds since the start of the FOS epoch
        on 1/1/2000 UTC'''
        
        fos_epoch = datetime(2000, 1, 1, 0, 0, 0, 0);
        #print fos_epoch

        #print datetime.utcnow()

        dt = datetime.utcnow()-fos_epoch
        return dt.days*24*60*60 + dt.seconds

def utc2fos(utc):
        '''Return time in seconds since the start of the FOS epoch
        on 1/1/2000 UTC until time'''
        
        fos_epoch = datetime(2000, 1, 1, 0, 0, 0, 0);
        #print fos_epoch

        #print datetime.utcnow()

        dt = utc-fos_epoch
        return dt.days*24*60*60 + dt.seconds

def fos2utc(ft):
        '''Convert FOS time in seconds since the start of the FOS epoch
        to local time.

        FOS time is in UTC, so the timezone conversion is done by the
        instantaneous difference between local and UTC time.  This is good
        for times around now, but may fail if you have daylight savings and
        the times involved are not current'''
        
        fos_epoch = datetime(2000, 1, 1, 0, 0, 0, 0);
        #print fos_e
        tzUTC = UTC()

        utc = fos_epoch + timedelta(seconds=ft);
        return utc.replace(tzinfo=tzUTC);

def fos2local(ft):
        '''Convert FOS time in seconds since the start of the FOS epoch
        to local time.

        FOS time is in UTC, so the timezone conversion is done by the
        instantaneous difference between local and UTC time.  This is good
        for times around now, but may fail if you have daylight savings and
        the times involved are not current'''
        
        fos_epoch = datetime(2000, 1, 1, 0, 0, 0, 0);
        #print fos_epoch

        tzUTC = UTC()
        tzLocal = LocalTimezone()

        utc = fos_epoch + timedelta(seconds=ft);
        utc = utc.replace(tzinfo=tzUTC);
        return utc.astimezone(tzLocal);

        
if 0:
        import FOSRPC

        s = Listener.Listener()
        s.connect(group=10, addr=0xffff, type=0xe9)

        rpc = FOSRPC.FOSRPC(s)
        
        b = rpc.rtc_get();
        print(b.time);
        print(fos2utc(b.time));

        t = now2fos();
        print(t)
        print(fos2utc(t))
        print(fos2local(t))


class TestFosTime(unittest.TestCase):
    
    def setUp(self):
        pass

    def test_numbers_3_4(self):
        self.assertEqual( multiply(3,4), 12)

    def test_strings_a_3(self):
        self.assertEqual( multiply('a',3), 'aaa')

if __name__ == '__main__':
    unittest.main()