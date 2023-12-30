import boto3
import gzip
import json
import os
import requests
import hashlib
import zipfile
import time
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

def cloudfront_invalidate(distribution_id):
	cf = boto3.client('cloudfront')
	res = cf.create_invalidation(DistributionId=distribution_id, InvalidationBatch={
		'Paths': {'Quantity': 1, 'Items': ['/*']}, 'CallerReference': str(time.time()).replace(".", "")
	})

	print("INVALIDATE '%s' ID '%s'" % (distribution_id, res['Invalidation']['Id']))

def output_set_hash(out, mty_plat, arch, core, data):
	if not out.get(mty_plat):
		out[mty_plat] = {}

	if not out[mty_plat].get(arch):
		out[mty_plat][arch] = {}

	new_hash = hashlib.sha256(data).hexdigest()

	if new_hash != out[mty_plat][arch].get(core, ''):
		out['id'] += 1

	out[mty_plat][arch][core] = new_hash

if __name__ == '__main__':
	cmd = 'sync'

	if len(sys.argv) > 1:
		cmd = sys.argv[1]

	script_path = os.path.dirname(os.path.realpath(__file__))
	core_hash_json_path = os.path.join(script_path, 'core-hash.json')
	core_hash_path = os.path.join(script_path, 'core-hash.h')

	try:
		out = json.loads(open(core_hash_json_path, 'r').read())
	except:
		out = {'id': 0}

	if cmd == 'upload':
		core = sys.argv[2]
		mty_plat = sys.argv[3]
		arch = sys.argv[4]
		file_name = sys.argv[5]

		data = open(file_name, 'rb').read()

		upload_to_s3(mty_plat, arch, file_name, data)
		output_set_hash(out, mty_plat, arch, core, data)

	elif cmd == 'sync':
		for platform, archs in platforms.items():
			for arch in archs:
				for core in cores:
					mty_plat = mty_platform[platform]

					fname = '%s_libretro.%s' % (core, suffix[platform])
					url = 'https://buildbot.libretro.com/nightly/%s/%s/latest/%s.zip' % (platform, arch, fname)
					r = requests.get(url, headers={'User-Agent': user_agent})

					print(r.status_code, url)

					if r.status_code == 200:
						zinput = zipfile.ZipFile(io.BytesIO(r.content))

						for name in zinput.namelist():
							if name == fname:
								data = zinput.read(name)
								upload_to_s3(mty_plat, arch, name, data)
								output_set_hash(out, mty_plat, arch, core, data)
								break

	cloudfront_invalidate('E2ULM8DUHAWNQK')

	open(core_hash_json_path, 'w').write(json.dumps(out, indent='\t', separators=(',', ': ')))
	open(core_hash_path, 'w').write('static const char *CORE_HASH = %s;\n' % json.dumps(json.dumps(out)))
