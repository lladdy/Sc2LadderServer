#include "Webservice.h"

#include "Types.h
#include "LadderConfig.h"

#ifdef ENABLE_CURL
#include "curl.h"
#endif

bool Webservice::UploadMime(LadderConfig& Config, ResultType result, const Matchup &ThisMatch)
{
#ifndef ENABLE_CURL
    return true;
#else
    std::string ReplayDir = Config->GetValue("LocalReplayDirectory");
	std::string UploadResultLocation = Config->GetValue("UploadResultLocation");
	std::string RawMapName = RemoveMapExtension(ThisMatch.Map);
	std::string ReplayFile;
	if (ThisMatch.Agent2.Type == BotType::DefaultBot)
	{
		ReplayFile = ThisMatch.Agent1.BotName + "v" + GetDifficultyString(ThisMatch.Agent2.Difficulty) + "-" + RawMapName + ".Sc2Replay";
	}
	else
	{
		ReplayFile = ThisMatch.Agent1.BotName + "v" + ThisMatch.Agent2.BotName + "-" + RawMapName + ".Sc2Replay";
	}
	ReplayFile.erase(remove_if(ReplayFile.begin(), ReplayFile.end(), isspace), ReplayFile.end());
	std::string ReplayLoc = ReplayDir + ReplayFile;
	CURL *curl;
	CURLcode res;



	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (curl) {
		curl_mime *form = NULL;
		curl_mimepart *field = NULL;
		struct curl_slist *headerlist = NULL;
		static const char buf[] = "Expect:";

		/* Create the form */
		form = curl_mime_init(curl);

		/* Fill in the file upload field */
		if (std::fstream(ReplayLoc.c_str()))
		{
			field = curl_mime_addpart(form);
			curl_mime_name(field, "replayfile");
			curl_mime_filedata(field, ReplayLoc.c_str());
		}
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
		/* Fill in the filename field */
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Bot1Name");
		curl_mime_data(field, ThisMatch.Agent1.BotName.c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Bot1Race");
		curl_mime_data(field, std::to_string((int)ThisMatch.Agent1.Race).c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Bot2Name");
		curl_mime_data(field, ThisMatch.Agent2.BotName.c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Bot2Race");
		curl_mime_data(field, std::to_string((int)ThisMatch.Agent2.Race).c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Map");
		curl_mime_data(field, RawMapName.c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "Result");
		curl_mime_data(field, GetResultType(result).c_str(), CURL_ZERO_TERMINATED);
		/* initialize custom header list (stating that Expect: 100-continue is not
		wanted */
		headerlist = curl_slist_append(headerlist, buf);
		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_URL, UploadResultLocation.c_str());

		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the form */
		curl_mime_free(form);
		/* free slist */
		curl_slist_free_all(headerlist);
		MoveReplayFile(ReplayLoc.c_str(), std::string(ReplayDir + "Uploaded/" + ReplayFile.c_str()).c_str());

		return true;
	}
	return false;
#endif
}

bool Webservice::LoginToServer(LadderConfig& Config)
{
#ifndef ENABLE_CURL
    return true;
#else
    CURL *curl;
	CURLcode res;

	curl_mime *form = NULL;
	curl_mimepart *field = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (curl) {
		/* Create the form */
		form = curl_mime_init(curl);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
		/* Fill in the filename field */
		field = curl_mime_addpart(form);
		curl_mime_name(field, "username");
		curl_mime_data(field, Config->GetValue("ServerUsername").c_str(), CURL_ZERO_TERMINATED);
		field = curl_mime_addpart(form);
		curl_mime_name(field, "password");
		curl_mime_data(field, Config->GetValue("ServerPassword").c_str(), CURL_ZERO_TERMINATED);

		/* initialize custom header list (stating that Expect: 100-continue is not
		wanted */
		headerlist = curl_slist_append(headerlist, buf);
		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_URL, Config->GetValue("ServerLoginAddress").c_str());

		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}


		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the form */
		curl_mime_free(form);
		/* free slist */
		curl_slist_free_all(headerlist);
		return true;
	}
	return false;
#endif
}