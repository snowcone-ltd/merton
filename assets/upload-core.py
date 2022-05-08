import boto3
import gzip
import sys
import os
import ntpath

boto3.setup_default_session(profile_name='default', region_name='us-east-1')

if __name__ == '__main__':
	file_name = sys.argv[1]

	gz_tmp_name = '%s.gz' % file_name
	gz_tmp = gzip.open(gz_tmp_name, 'wb')
	gz_tmp.write(open(file_name, 'rb').read())
	gz_tmp.close()

	gz_tmp = open(gz_tmp_name, 'rb')

	s3 = boto3.client('s3')
	s3.put_object(ACL='public-read', Body=gz_tmp, Bucket='merton.matoya.group',
		Key='cores/' + os.path.basename(file_name), ContentEncoding='gzip')

	gz_tmp.close()
	os.remove(gz_tmp_name)
