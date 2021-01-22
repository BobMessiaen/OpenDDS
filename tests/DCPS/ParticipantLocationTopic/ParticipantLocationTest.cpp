/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include "dds/DCPS/StaticIncludes.h"

#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#  ifdef OPENDDS_SECURITY
#  include "dds/DCPS/security/BuiltInPlugins.h"
#  endif
# endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include "ParticipantLocationListenerImpl.h"

#include "tests/Utils/StatusMatching.h"

#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>

const char auth_ca_file_from_tests[] = "security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file_from_tests[] = "security/certs/identity/test_participant_01_cert.pem";
const char id_key_file_from_tests[] = "security/certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";
#endif

bool no_ice = false;
bool ipv6 = false;

int parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT ("n6"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'n':
        no_ice = true;
        break;
      case '6':
        ipv6 = true;
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-n do not check for ICE connections"
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

ACE_Thread_Mutex participants_done_lock;
ACE_Condition_Thread_Mutex participants_done_cond(participants_done_lock);
int participants_done = 0;

void participants_done_callback()
{
  ACE_Guard<ACE_Thread_Mutex> g(participants_done_lock);
  ++participants_done;
  participants_done_cond.signal();
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;

  try {
    std::cout << "Starting publisher" << std::endl;

    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    if( parse_args(argc, argv) != 0)
      return 1;

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

    DDS::PropertySeq& props = part_qos.property.value;
    append(props, "OpenDDS.RtpsRelay.Groups", "Messenger", true);

#ifdef OPENDDS_SECURITY
      // Determine the path to the keys
      OPENDDS_STRING path_to_tests;
      const char* dds_root = ACE_OS::getenv("DDS_ROOT");
      if (dds_root && dds_root[0]) {
        // Use DDS_ROOT in case we are one of the CMake tests
        path_to_tests = OPENDDS_STRING("file:") + dds_root + "/tests/";
      } else {
        // Else if DDS_ROOT isn't defined try to do it relative to the traditional location
        path_to_tests = "file:../../";
      }
      const OPENDDS_STRING auth_ca_file = path_to_tests + auth_ca_file_from_tests;
      const OPENDDS_STRING perm_ca_file = path_to_tests + perm_ca_file_from_tests;
      const OPENDDS_STRING id_cert_file = path_to_tests + id_cert_file_from_tests;
      const OPENDDS_STRING id_key_file = path_to_tests + id_key_file_from_tests;
      if (TheServiceParticipant->get_security()) {
        append(props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file.c_str());
        append(props, DDS::Security::Properties::AuthIdentityCertificate, id_cert_file.c_str());
        append(props, DDS::Security::Properties::AuthPrivateKey, id_key_file.c_str());
        append(props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file.c_str());
        append(props, DDS::Security::Properties::AccessGovernance, governance_file);
        append(props, DDS::Security::Properties::AccessPermissions, permissions_file);
      }
#endif

    // Create Publisher DomainParticipant
    DDS::DomainParticipant_var participant = dpf->create_participant(4,
                                                                     part_qos,
                                                                     DDS::DomainParticipantListener::_nil(),
                                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       EXIT_FAILURE);
    }

    // Get the Built-In Subscriber for Built-In Topics
    DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();

    DDS::DataReader_var pub_loc_dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (0 == pub_loc_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    ParticipantLocationListenerImpl* listener = new ParticipantLocationListenerImpl("Publisher", no_ice, ipv6, participants_done_callback);
    DDS::DataReaderListener_var listener_var(listener);

    CORBA::Long retcode =
      pub_loc_dr->set_listener(listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    // Create Subscriber DomainParticipant
    DDS::DomainParticipant_var sub_participant = dpf->create_participant(4,
                                                                     part_qos,
                                                                     DDS::DomainParticipantListener::_nil(),
                                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(sub_participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: subsriber create_participant failed!\n")),
                       EXIT_FAILURE);
    }
    // Get the Built-In Subscriber for Built-In Topics
    DDS::Subscriber_var sub_bit_subscriber = sub_participant->get_builtin_subscriber();

    DDS::DataReader_var sub_loc_dr = sub_bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (0 == sub_loc_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    ParticipantLocationListenerImpl* sub_listener = new ParticipantLocationListenerImpl("Subscriber", no_ice, ipv6, participants_done_callback);
    DDS::DataReaderListener_var sub_listener_var(sub_listener);

    retcode =
      sub_loc_dr->set_listener(sub_listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    while (participants_done != 2) {
      participants_done_cond.wait();
    }

    // check that all locations received
    if (!listener->check()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Check for all publisher locations failed\n")));
      status = EXIT_FAILURE;
    }

    if (!sub_listener->check()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Check for all subscriber locations failed\n")));
      status = EXIT_FAILURE;
    }

    // Clean-up!
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" publisher participant deleting contained entities\n")));
    participant->delete_contained_entities();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" subscriber participant deleting contained entities\n")));
    sub_participant->delete_contained_entities();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" domain participant factory deleting participants\n")));
    dpf->delete_participant(participant.in());
    dpf->delete_participant(sub_participant.in());
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" shutdown service participant\n")));
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
