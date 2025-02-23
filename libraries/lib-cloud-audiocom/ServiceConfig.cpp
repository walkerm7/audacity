/*  SPDX-License-Identifier: GPL-2.0-or-later */
/*!********************************************************************

  Audacity: A Digital Audio Editor

  ServiceConfig.cpp

  Dmitry Vedenko

**********************************************************************/
#include "ServiceConfig.h"
#include "Languages.h"

namespace cloud::audiocom
{
std::string_view ServiceConfig::GetAPIEndpoint() const
{
   return "https://api.audio.com";
}

std::string_view ServiceConfig::GetOAuthLoginPage() const
{
   static const std::string loginPage =
      std::string("https://audio.com/audacity/link?clientId=") +
      std::string(GetOAuthClientID());

   return loginPage;
}

std::string_view ServiceConfig::GetOAuthClientID() const
{
   return "1741964426607541";
}

std::string_view ServiceConfig::GetOAuthRedirectURL() const
{
   //return "audacity://link";
   return "https://audio.com/auth/sign-in/success";
}

std::string ServiceConfig::GetAPIUrl(std::string_view apiURI) const
{
   return std::string(GetAPIEndpoint()) + std::string(apiURI);
}

std::string ServiceConfig::GetFinishUploadPage(
   std::string_view audioID, std::string_view token) const
{
   return "https://audio.com/audacity/upload?audioId=" + std::string(audioID) +
          "&token=" + std::string(token) +
          "&clientId=" + std::string(GetOAuthClientID());
}

std::string ServiceConfig::GetAudioURL(
   std::string_view userSlug, std::string_view audioSlug) const
{
   return "https://audio.com/" + std::string(userSlug) + "/audio/" + std::string(audioSlug) + "/edit";
}

std::chrono::milliseconds ServiceConfig::GetProgressCallbackTimeout() const
{
   return std::chrono::seconds(3);
}

MimeTypesList ServiceConfig::GetPreferredAudioFormats() const
{
   return { "audio/x-wavpack", "audio/x-flac", "audio/x-wav" };
}

MimeType ServiceConfig::GetDownloadMime() const
{
   return "audio/x-wav";
}

std::string ServiceConfig::GetAcceptLanguageValue() const
{
   auto language = Languages::GetLang();

   if (language.Contains(L"-") && language.Length() > 2)
      return wxString::Format("%s;q=1.0, %s;q=0.7, *;q=0.5", language, language.Left(2)).ToStdString();
   else
      return wxString::Format("%s;q=1.0, *;q=0.5", language).ToStdString();
}

const ServiceConfig& GetServiceConfig()
{
   static ServiceConfig config;
   return config;
}

} // namespace cloud::audiocom
