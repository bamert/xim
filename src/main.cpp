#include <iostream>
#include <curl/curl.h>
using namespace std;


static char errorBuffer[CURL_ERROR_SIZE];
static std::string buffer;

//
//  libcurl write callback function
//

static int writer(char *data, size_t size, size_t nmemb,
                  std::string *writerData) {
  if (writerData == NULL)
    return 0;

  writerData->append(data, size * nmemb);

  return size * nmemb;
}
static bool init(CURL *&conn, char *url) {
  CURLcode code;

  conn = curl_easy_init();

  if (conn == NULL) {
    fprintf(stderr, "Failed to create CURL connection\n");
    exit(EXIT_FAILURE);
  }

  code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);
  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to set error buffer [%d]\n", code);
    return false;
  }

  code = curl_easy_setopt(conn, CURLOPT_URL, url);
  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to set URL [%s]\n", errorBuffer);
    return false;
  }

  code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);
  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to set redirect option [%s]\n", errorBuffer);
    return false;
  }

  code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);
  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to set writer [%s]\n", errorBuffer);
    return false;
  }

  code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, &buffer);
  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to set write data [%s]\n", errorBuffer);
    return false;
  }

  return true;
}
int main(int argc, char *argv[]) {
  cout << "ximiner alive" << endl;
  CURL *conn = NULL;
  CURLcode code;
  std::string title;

  // Ensure one argument is given

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <url>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Initialize CURL connection

  if (!init(conn, argv[1])) {
    fprintf(stderr, "Connection initializion failed\n");
    exit(EXIT_FAILURE);
  }

  // Retrieve content for the URL

  code = curl_easy_perform(conn);
  curl_easy_cleanup(conn);

  if (code != CURLE_OK) {
    fprintf(stderr, "Failed to get '%s' [%s]\n", argv[1], errorBuffer);
    exit(EXIT_FAILURE);
  }
  cout << buffer << endl;
}