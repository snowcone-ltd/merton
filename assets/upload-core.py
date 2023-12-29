import boto3
import gzip
import json
import os
import requests
import hashlib
import zipfile
import io
import sys

user_agent = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'

platforms = {
	'linux' : ['x86_64'],
	'apple/osx' : ['x86_64', 'arm64'],
	'windows': ['x86_64'],
}

suffix = {
	'linux': 'so',
	'apple/osx': 'dylib',
	'windows': 'dll',
}

mty_platform = {
	'linux': 'linux',
	'apple/osx': 'macosx',
	'windows': 'windows',
}

cores = [
	'stella',
	'sameboy',
	'mgba',
	'genesis_plus_gx',
	'mupen64plus_next',
	'mesen',
	'swanstation',
	'bsnes',
	'mesen-s',
	'snes9x',
	'mednafen_pce',
]

custom_cores = [
	'merton-nes',
]

boto3.setup_default_session(profile_name='default', region_name='us-east-1')

def upload_to_s3(platform, arch, file_name, data):
	file_name = file_name.replace('_libretro', '')

	gz_tmp_name = '%s.gz' % file_name
	gz_tmp = gzip.open(gz_tmp_name, 'wb')
	gz_tmp.write(data)
	gz_tmp.close()

	gz_tmp = open(gz_tmp_name, 'rb')

	s3 = boto3.client('s3')
	s3_path = 'cores/%s/%s/%s' % (platform, arch, os.path.basename(file_name))
	s3.put_object(ACL='public-read', Body=gz_tmp, Bucket='www.snowcone.ltd',
		Key=s3_path, ContentEncoding='gzip')

	gz_tmp.close()
	os.remove(gz_tmp_name)

if __name__ == '__main__':
	cmd = 'sync'

	if len(sys.argv) > 1:
		cmd = sys.argv[1]

	if cmd == 'upload':
		platform = sys.argv[2]
		arch = sys.argv[3]
		name = sys.argv[4]

		upload_to_s3(platform, arch, name, open(name, 'rb').read())

	elif cmd == 'hash':
		out = {}

		for platform, archs in platforms.items():
			for arch in archs:
				for core in cores + custom_cores:
					mty_plat = mty_platform[platform]
					url = 'https://snowcone.ltd/cores/%s/%s/%s.%s' % (mty_plat, arch, core, suffix[platform])
					r = requests.get(url, headers={'User-Agent': user_agent})

					if not out.get(mty_plat):
						out[mty_plat] = {}

					if not out[mty_plat].get(arch):
						out[mty_plat][arch] = {}

					print(r.status_code, url)

					if r.status_code == 200:
						out[mty_plat][arch][core] = hashlib.sha256(r.content).hexdigest()

		j = json.dumps(json.dumps(out))
		open('core-hash.h', 'w').write('static const char *CORE_HASH = %s' % j)

	elif cmd == 'sync':
		for platform, archs in platforms.items():
			for arch in archs:
				for core in cores:
					fname = '%s_libretro.%s.zip' % (core, suffix[platform])
					url = 'https://buildbot.libretro.com/nightly/%s/%s/latest/%s' % (platform, arch, fname)
					r = requests.get(url, headers={'User-Agent': user_agent})

					print(r.status_code, url)

					if r.status_code == 200:
						zinput = zipfile.ZipFile(io.BytesIO(r.content))

						for name in zinput.namelist():
							upload_to_s3(mty_platform[platform], arch, name, zinput.read(name))
