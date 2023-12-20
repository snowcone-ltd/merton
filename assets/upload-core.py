import boto3
import gzip
import os
import requests
import zipfile
import io

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
	'apple/osx': 'macos',
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

if __name__ == '__main__':
	boto3.setup_default_session(profile_name='default', region_name='us-east-1')

	for platform, archs in platforms.items():
		for arch in archs:
			for core in cores:
				fname = '%s_libretro.%s.zip' % (core, suffix[platform])
				furl = 'https://buildbot.libretro.com/nightly/%s/%s/latest/%s' % (platform, arch, fname)
				r = requests.get(furl, headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'})

				print(r.status_code, furl)

				if r.status_code == 200:
					zinput = zipfile.ZipFile(io.BytesIO(r.content))

					for name in zinput.namelist():
						file_name = name.replace('_libretro', '')

						gz_tmp_name = '%s.gz' % file_name
						gz_tmp = gzip.open(gz_tmp_name, 'wb')
						gz_tmp.write(zinput.read(name))
						gz_tmp.close()

						gz_tmp = open(gz_tmp_name, 'rb')

						s3 = boto3.client('s3')
						s3_path = 'cores/%s/%s/%s' % (mty_platform[platform], arch, os.path.basename(file_name))
						s3.put_object(ACL='public-read', Body=gz_tmp, Bucket='www.snowcone.ltd',
							Key=s3_path, ContentEncoding='gzip')

						gz_tmp.close()
						os.remove(gz_tmp_name)
