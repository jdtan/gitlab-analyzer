import threading
from typing import Union, Tuple

import gitlab

from interface.gitlab_analyzer_interface import GitLabAnalyzer
from interface.gitlab_project_interface import GitLabProject

ERROR_CODES = {
    "invalidToken": "Invalid token",
    "invalidProjectID": "Invalid project ID",
}


class GitLabAnalyzerManager:
    def __init__(self):
        self.__gitlab_list: dict = {}

    # authenticate and add the gitlab instance on success
    def add_gitlab(self, token: str, hashedToken: str, url: str) -> Tuple[bool, str]:
        try:
            myGitLabAnalyzer = GitLabAnalyzer(token, hashedToken, url)
            self.__gitlab_list[hashedToken] = myGitLabAnalyzer
            return True, ""
        except gitlab.exceptions.GitlabAuthenticationError:
            return False, ERROR_CODES["invalidToken"]

    def __find_gitlab(self, hashedToken: str) -> Union[GitLabAnalyzer]:
        return self.__gitlab_list.get(hashedToken, None)

    def get_project_list(self, hashedToken: str) -> Tuple[bool, str, list]:
        myGitLab = self.__find_gitlab(hashedToken)
        if myGitLab is not None:
            return True, "", myGitLab.get_all_gitlab_project_name()
        else:
            return False, ERROR_CODES["invalidToken"], []

    def __sync_project_helper(self, projectID: int, myAnalyzer: GitLabAnalyzer) -> None:
        myAnalyzer.update_project(projectID)

    def __check_if_valid_token(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, Union[GitLabAnalyzer, None], Union[GitLabProject, None]]:
        myGitLab = self.__find_gitlab(hashedToken)
        if myGitLab is None:
            return False, ERROR_CODES["invalidToken"], myGitLab, None

        myProject = myGitLab.get_gitlab_project_by_id(projectID)
        if myProject is None:
            return False, ERROR_CODES["invalidProjectID"], myGitLab, myProject
        return True, "", myGitLab, myProject

    def sync_project(self, hashedToken: str, projectID: int) -> Tuple[bool, str]:
        isValid, errorCode, myGitLab, _ = self.__check_if_valid_token(
            hashedToken, projectID
        )
        if isValid:
            threading.Thread(
                target=self.__sync_project_helper, args=(projectID, myGitLab)
            ).start()
        return isValid, errorCode

    def check_sync_state(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, dict]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        syncState: dict = {}
        if isValid:
            syncState = myProject.get_project_sync_state()
        return isValid, errorCode, syncState

    def get_project_members(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        projectMembers: list = []
        if isValid:
            projectMembers = myProject.get_members()
        return isValid, errorCode, projectMembers

    def get_project_users(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        userList: list = []
        if isValid:
            userList = myProject.user_list
        return isValid, errorCode, userList

    def get_project_master_commits(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        commitList: list = []
        if isValid:
            commitList = myProject.get_commit_list_on_master()
        return isValid, errorCode, commitList

    def get_project_all_commits_by_user(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        commitList: list = []
        if isValid:
            commitList = myProject.get_commits_for_all_users()
        return isValid, errorCode, commitList

    def get_project_all_merge_request(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, dict]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        mergeRequestList: dict = {}
        if isValid:
            mergeRequestList = myProject.get_merge_request_and_commit_list_for_users()
        return isValid, errorCode, mergeRequestList

    def get_project_merge_request_by_user(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        mergeRequestList: list = []
        if isValid:
            mergeRequestList = myProject.get_all_merge_request_and_commit()
        return isValid, errorCode, mergeRequestList

    def get_code_diff(
        self, hashedToken: str, projectID: int, codeDiffID
    ) -> Tuple[bool, str, dict]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        codeDiff: dict = {}
        if isValid:
            codeDiff = myProject.get_code_diff(codeDiffID)
        return isValid, errorCode, codeDiff

    def get_project_comments(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, list]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        commentList: list = []
        if isValid:
            commentList = myProject.get_all_comments()
        return isValid, errorCode, commentList

    def get_project_comments_by_user(
        self, hashedToken: str, projectID: int
    ) -> Tuple[bool, str, dict]:
        isValid, errorCode, _, myProject = self.__check_if_valid_token(
            hashedToken, projectID
        )
        commentList: dict = {}
        if isValid:
            commentList = myProject.get_comments_for_all_users()
        return isValid, errorCode, commentList