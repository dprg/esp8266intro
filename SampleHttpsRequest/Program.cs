using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;

namespace SampleHttpsRequest
{
	class Program
	{
		static void Main(string[] args)
		{
			var ipAddress = System.Net.IPAddress.Parse("192.168.202.111");
			var request = JsonConvert.SerializeObject(
					new {
						Speed = 0,
						Brightness = 150
					}, Formatting.None);

			SendGetRequest("admin:esp8266", ipAddress, true, "strip");
			SendPostRequest("admin:esp8266", ipAddress, true, "strip", request);
		}

		/// <summary>
		/// Send a Get request
		/// </summary>
		/// <param name="authorization"></param>
		/// <param name="ipAddress"></param>
		/// <param name="verbose"></param>
		/// <param name="v"></param>
		/// <returns></returns>
		private static bool SendGetRequest(string authorization, IPAddress ipAddress, bool verbose, string path)
		{
			using (var client = new HttpClient())
			{
				var authByteArray = Encoding.ASCII.GetBytes(authorization);

				client.BaseAddress = new Uri($"https://{ipAddress}/");
				client.DefaultRequestHeaders.Accept.Clear();
				client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
				client.DefaultRequestHeaders.Authorization =
					new AuthenticationHeaderValue("Basic", Convert.ToBase64String(authByteArray));

				if (verbose)
				{
					var currentForeground = Console.ForegroundColor;
					Console.ForegroundColor = ConsoleColor.Cyan;
					Console.WriteLine($"Verbose: Client Base Address = {client.BaseAddress}");
					Console.ForegroundColor = currentForeground;
				}

				// HTTP POST
				NEVER_EAT_POISON_Disable_CertificateValidation();
				try
				{
					var response = client.GetAsync(path).Result;

					if (response.IsSuccessStatusCode)
					{
						var data = response.Content.ReadAsStringAsync().Result;
						Console.WriteLine(data);
					}
					else
					{
						Console.WriteLine(response.StatusCode);
					}
				}
				catch (Exception e)
				{
					while (e.InnerException != null) e = e.InnerException;
					Console.WriteLine(e.Message);
					return false;
				}
			}

			return true;
		}

		/// <summary>
		/// Send a Post request
		/// </summary>
		/// <param name="authorization"></param>
		/// <param name="ipAddress"></param>
		/// <param name="verbose"></param>
		/// <param name="path"></param>
		/// <param name="request"></param>
		/// <returns></returns>
		private static bool SendPostRequest(string authorization, IPAddress ipAddress, bool verbose, string path, string request)
		{
			using (var client = new HttpClient())
			{
				var authByteArray = Encoding.ASCII.GetBytes(authorization);

				client.BaseAddress = new Uri($"https://{ipAddress}/");
				client.DefaultRequestHeaders.Accept.Clear();
				client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
				client.DefaultRequestHeaders.Authorization =
					new AuthenticationHeaderValue("Basic", Convert.ToBase64String(authByteArray));

				if (verbose)
				{
					var currentForeground = Console.ForegroundColor;
					Console.ForegroundColor = ConsoleColor.Cyan;
					Console.WriteLine($"Verbose: Client Base Address = {client.BaseAddress}");
					Console.WriteLine($"Request:");
					Console.WriteLine(request);
					Console.ForegroundColor = currentForeground;
				}

				// if you are sending some object directly, this is how you would serialize it
				//var content = new StringContent(JsonConvert.SerializeObject(signSettingsRequest.ToString()));
				using (var content = new StringContent(request))
				{
					// HTTP POST
					NEVER_EAT_POISON_Disable_CertificateValidation();
					try
					{
						var response = client.PostAsync(path, content).Result;

						if (response.IsSuccessStatusCode)
						{
							var data = response.Content.ReadAsStringAsync().Result;
							Console.WriteLine(data);
						}
						else
						{
							Console.WriteLine(response.StatusCode);
						}
					}
					catch (Exception e)
					{
						while (e.InnerException != null) e = e.InnerException;
						Console.WriteLine(e.Message);
						return false;
					}
				}
			}

			return true;
		}

		/// <summary>
		/// This is used to bypass certificate validation for self-signed certs..
		/// Use CA signed certificates and NEVER use this in production code.
		/// </summary>
		[Obsolete("Do not use this in Production code!!!", false)]
		static void NEVER_EAT_POISON_Disable_CertificateValidation()
		{
			// Disabling certificate validation can expose you to a man-in-the-middle attack
			// which may allow your encrypted message to be read by an attacker
			// https://stackoverflow.com/a/14907718/740639
			ServicePointManager.ServerCertificateValidationCallback =
				(s, certificate, chain, sslPolicyErrors) => true;
		}

	}
}
