/* SecretAgent.c generated by valac 0.14.0, the Vala compiler
 * generated from SecretAgent.vala, do not modify */


#include <glib.h>
#include <glib-object.h>
#include <nm-secret-agent.h>

#define UNITY_SETTINGS_TYPE_SECRET_AGENT (unity_settings_secret_agent_get_type ())
#define UNITY_SETTINGS_SECRET_AGENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_SETTINGS_TYPE_SECRET_AGENT, UnitySettingsSecretAgent))
#define UNITY_SETTINGS_SECRET_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_SETTINGS_TYPE_SECRET_AGENT, UnitySettingsSecretAgentClass))
#define UNITY_SETTINGS_IS_SECRET_AGENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_SETTINGS_TYPE_SECRET_AGENT))
#define UNITY_SETTINGS_IS_SECRET_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_SETTINGS_TYPE_SECRET_AGENT))
#define UNITY_SETTINGS_SECRET_AGENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_SETTINGS_TYPE_SECRET_AGENT, UnitySettingsSecretAgentClass))

typedef struct _UnitySettingsSecretAgent UnitySettingsSecretAgent;
typedef struct _UnitySettingsSecretAgentClass UnitySettingsSecretAgentClass;
typedef struct _UnitySettingsSecretAgentPrivate UnitySettingsSecretAgentPrivate;

struct _UnitySettingsSecretAgent {
	NMSecretAgent parent_instance;
	UnitySettingsSecretAgentPrivate * priv;
};

struct _UnitySettingsSecretAgentClass {
	NMSecretAgentClass parent_class;
};


static gpointer unity_settings_secret_agent_parent_class = NULL;

GType unity_settings_secret_agent_get_type (void) G_GNUC_CONST;
enum  {
	UNITY_SETTINGS_SECRET_AGENT_DUMMY_PROPERTY
};

UnitySettingsSecretAgent* unity_settings_secret_agent_new       (void);
UnitySettingsSecretAgent* unity_settings_secret_agent_construct (GType object_type);


UnitySettingsSecretAgent*
unity_settings_secret_agent_construct (GType object_type)
{
	UnitySettingsSecretAgent * self = NULL;
	self = (UnitySettingsSecretAgent*) g_object_new (object_type,
                                                         NM_SECRET_AGENT_IDENTIFIER, "com.unity.nm-agent",
                                                         NULL);
	return self;
}


UnitySettingsSecretAgent*
unity_settings_secret_agent_new (void)
{
	return unity_settings_secret_agent_construct (UNITY_SETTINGS_TYPE_SECRET_AGENT);
}


static void
unity_settings_secret_agent_class_init (UnitySettingsSecretAgentClass *klass)
{
	unity_settings_secret_agent_parent_class = g_type_class_peek_parent (klass);
}


static void
unity_settings_secret_agent_instance_init (UnitySettingsSecretAgent *self)
{
}


GType unity_settings_secret_agent_get_type (void) {
	static volatile gsize unity_settings_secret_agent_type_id__volatile = 0;
	if (g_once_init_enter (&unity_settings_secret_agent_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (UnitySettingsSecretAgentClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) unity_settings_secret_agent_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (UnitySettingsSecretAgent), 0, (GInstanceInitFunc) unity_settings_secret_agent_instance_init, NULL };
		GType unity_settings_secret_agent_type_id;
		unity_settings_secret_agent_type_id = g_type_register_static (G_TYPE_OBJECT, "UnitySettingsSecretAgent", &g_define_type_info, 0);
		g_once_init_leave (&unity_settings_secret_agent_type_id__volatile, unity_settings_secret_agent_type_id);
	}
	return unity_settings_secret_agent_type_id__volatile;
}



