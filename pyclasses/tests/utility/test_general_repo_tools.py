import unittest
# Import functions that I want to test.
from utility.general_repo_tools import get_repo_root
from utility.general_repo_tools import get_all_applications
from utility.general_repo_tools import get_application_for_given_target
from utility.general_repo_tools import get_targets_for_application
from utility.general_repo_tools import get_all_targets_in_repo


class TestGeneralRepoTools(unittest.TestCase):

    def test_repo_root(self):
        # Just test if the repo name is pacp-freertos. I can't imagine this
        # will ever change and probably is a good indicator that this function
        # is doing it's job.
        repo_name = get_repo_root().split("/")[-1]
        self.assertEqual(repo_name, "pacp-freertos")

    def test_applications(self):
        # Get all applications has it's paths hard coded to apps and test_apps
        # If it can't find apps to test, other tools will break too, so check
        # this this will always return something.
        applications = get_all_applications()
        self.assertEqual(type(applications), list)
        self.assertNotEqual(len(applications), 0)

    def test_get_application_for_target(self):
        # Probably a safe bet this target doesn't exist.
        application = get_application_for_given_target("bingo-boingo")
        self.assertEqual(application, None)
        application = get_application_for_given_target("ceresat")
        self.assertNotEqual(application, None)

    def test_get_targets_for_application(self):
        # A safe bet this app doesn't exist, which should raise an Exception
        self.assertRaises(Exception, get_targets_for_application, "bingo-boingo")
        # Probably a safe bet that unifiedbase will always exist.
        # If for some reason it doesn't or the name changes, change this to an
        # app with more than a single target.
        targets = get_targets_for_application("unifiedbase")
        self.assertNotEqual(targets, None)
        self.assertTrue(len(targets) > 1)

    def test_get_all_targets_in_repo(self):
        # Test that there is more than a single target in the repo. If this
        # isn't more than 1, something is probably wrong.
        targets = get_all_targets_in_repo()
        self.assertEqual(type(targets), list)
        self.assertTrue(len(targets) > 1)


if __name__ == '__main__':
    unittest.main()
