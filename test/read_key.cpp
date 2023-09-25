#include <openssl/store.h>
#include <openssl/types.h>
#include <openssl/ui.h>

#define SET_EXPECT(expect, val) ((expect) = (expect) < 0 ? (val) : ((expect) == (val) ? (val) : 0))

static class UserInterface {
	public:
		UserInterface(const char * password = nullptr) : m_password(password) {}
		~UserInterface() = default;

		static UI_METHOD * create() {
			if (UserInterface::ms_ui_base_method == nullptr)
				UserInterface::ms_ui_base_method = UI_OpenSSL();

			UI_METHOD * ui_method = UI_create_method("Test application");
			UI_method_set_closer(ui_method, UserInterface::close);
			UI_method_set_opener(ui_method, UserInterface::open);
			UI_method_set_prompt_constructor(ui_method, UserInterface::prompt);
			UI_method_set_reader(ui_method, UserInterface::read);
			UI_method_set_writer(ui_method, UserInterface::write);
			return ui_method;
		}

	private:
		static int close(UI * ui) {
			int (*closer)(UI * ui) = UI_method_get_closer(UserInterface::ms_ui_base_method);

			if (closer != nullptr)
				return closer(ui);
			else
				return 1;
		}

		static int open(UI * ui) {
			int (*opener)(UI * ui) = UI_method_get_opener(UserInterface::ms_ui_base_method);
			if (opener != nullptr)
				return opener(ui);
			else
				return 1;
		}

		static char * prompt(UI * ui, const char * phrase_desc, const char * object_name) {
			if (phrase_desc == nullptr)
				phrase_desc = "pass phrase";
			return UI_construct_prompt(nullptr, phrase_desc, object_name);
		}

		static int read(UI * ui, UI_STRING * uis) {
			int (*reader)(UI *ui, UI_STRING *uis) = nullptr;

			if (UI_get_input_flags(uis) & UI_INPUT_FLAG_DEFAULT_PWD && UI_get0_user_data(ui)) {
				switch (UI_get_string_type(uis)) {
					case UIT_PROMPT:
					case UIT_VERIFY: {
						UserInterface * user_interface = reinterpret_cast<UserInterface *>(UI_get0_user_data(ui));
						const char * password = user_interface->m_password;

						if (password != nullptr) {
							UI_set_result(ui, uis, password);
							return 1;
						}
						break;
					}

					case UIT_NONE:
					case UIT_BOOLEAN:
					case UIT_INFO:
					case UIT_ERROR:
						break;
				}
			}

			reader = UI_method_get_reader(UserInterface::ms_ui_base_method);
			if (reader != NULL)
				return reader(ui, uis);

			/* Default to the empty password if we've got nothing better */
			UI_set_result(ui, uis, "");
			return 1;
		}

		static int write(UI * ui, UI_STRING * uis) {
			int (*writer)(UI * ui, UI_STRING * uis) = nullptr;

			if (UI_get_input_flags(uis) & UI_INPUT_FLAG_DEFAULT_PWD && UI_get0_user_data(ui)) {
				switch (UI_get_string_type(uis)) {
					case UIT_PROMPT:
					case UIT_VERIFY: {
						UserInterface * user_interface = reinterpret_cast<UserInterface *>(UI_get0_user_data(ui));
						const char * password = user_interface->m_password;

						if (password != nullptr)
							return 1;
						break;
					}
					case UIT_NONE:
					case UIT_BOOLEAN:
					case UIT_INFO:
					case UIT_ERROR:
						break;
				}
			}

			writer = UI_method_get_writer(UserInterface::ms_ui_base_method);
			if (writer != nullptr)
				return writer(ui, uis);
			else
				return 1;
		}

		const char * m_password;
		static const UI_METHOD * ms_ui_base_method;
} user_interface;

const UI_METHOD * UserInterface::ms_ui_base_method = nullptr;

int main() {
	const char * filename = "/build/test/key.pub";
	OSSL_LIB_CTX * libctx = nullptr;
	const char * propq = nullptr;
	const OSSL_PARAM * params = NULL;

	OSSL_STORE_CTX *ctx = OSSL_STORE_open_ex(filename, libctx, propq, UI_OpenSSL(), nullptr, params, NULL, NULL);

	int expect = -1;
	SET_EXPECT(expect, OSSL_STORE_INFO_PKEY);
	// OSSL_STORE_expect(ctx, expect);

	while (!OSSL_STORE_eof(ctx)) {
		OSSL_STORE_INFO * info = OSSL_STORE_load(ctx);
		if (info == nullptr)
			continue;

		EVP_PKEY * ppkey = nullptr, * ppubkey = nullptr;
		int type = OSSL_STORE_INFO_get_type(info);
		switch (type) {
			case OSSL_STORE_INFO_PKEY:
				ppkey = OSSL_STORE_INFO_get1_PKEY(info);
				// EVP_PKEY_print_public(out, ppkey, 0, NULL)
				break;
			case OSSL_STORE_INFO_PUBKEY:
				ppubkey = OSSL_STORE_INFO_get1_PUBKEY(info);
				break;
		}
	}

	OSSL_STORE_close(ctx);

	return 0;
}