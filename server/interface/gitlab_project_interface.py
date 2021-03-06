from interface.gitlab_interface import GitLab
from manager.comment_manager import CommentManager
from manager.commit_manager import CommitManager
from manager.member_manager import MemberManager
from manager.merge_request_manager import MergeRequestManager
from manager.issue_manager import IssueManager


class GitLabProject:
    def __init__(self, myGitlab: GitLab, projectID: int):
        self.__gitlab: GitLab = myGitlab
        self.__gitlab.set_project(projectID=projectID)

        self.__membersManager: MemberManager = MemberManager()
        self.__issuesManager: IssueManager = IssueManager()
        self.__commitsManager: CommitManager = CommitManager()
        self.__commentsManager: CommentManager = CommentManager()
        self.__mergeRequestManager: MergeRequestManager = MergeRequestManager()

        self.__update_comment_manager()
        self.__update_merge_request_manager()
        self.__update_member_manager()
        self.__update_commits_manager()
        self.__update_issues_manager()

    def __update_comment_manager(self):
        # TODO: This needs to be discussed
        # https://python-gitlab.readthedocs.io/en/stable/gl_objects/notes.html
        pass

    def __update_merge_request_manager(self):
        mergeRequests, _ = self.__gitlab.get_merge_requests_and_commits(state="all")
        for mergeRequest in mergeRequests:
            self.__mergeRequestManager.add_merge_request(mergeRequest)

    def __update_member_manager(self):
        members: list = self.__gitlab.get_all_members()
        for member in members:
            self.__membersManager.add_member(member)
        pass

    def __update_commits_manager(self):
        commitList: list = self.__gitlab.get_commit_list_for_project()
        for commit in commitList:
            self.__commitsManager.add_commit(commit)

    def __update_issues_manager(self):
        issueList: list = self.__gitlab.get_issue_list()
        self.__issuesManager.populate_issue_list(issueList)

    def get_commits_for_all_users(self):
        commitListsForAllUsers = {}

        for commit in self.__commitsManager.get_commit_list():
            authorName = commit.author_name
            if commitListsForAllUsers.get(authorName) is None:
                commitListsForAllUsers[authorName] = []
            else:
                commitListsForAllUsers.get(authorName).append(commit.to_dict())

    def get_merge_request_and_commit_list(self):
        mergeRequestForAllUsers = []
        mrs, commits_lists = self.__gitlab.get_merge_requests_and_commits()
        for mr, commits in zip(mrs, commits_lists):
            data = mr.to_dict()
            commitList = []
            for commit in commits:
                commitList.append(commit.to_dict())
            data["commit_list"] = commitList
            mergeRequestForAllUsers.append(data)
        

    @property
    def project_list(self) -> list:
        return self.__gitlab.get_project_list()

    @property
    def member_manager(self) -> MemberManager:
        return self.__membersManager

    @property
    def merge_request_manager(self) -> MergeRequestManager:
        return self.__mergeRequestManager

    @property
    def commits_manager(self) -> CommitManager:
        return self.__commitsManager
