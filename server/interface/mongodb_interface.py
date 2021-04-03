from model.issue import Issue
from typing import List, NamedTuple, Tuple

from pymongo import MongoClient
from pymongo.collection import Collection
from pymongo.cursor import Cursor
from pymongo.errors import DuplicateKeyError
from pymongo.results import *

from model.commit import Commit
from model.merge_request import MergeRequest
from model.comment import Comment
from model.member import Member
from interface.gitlab_project_interface import GitLabProject

from datetime import datetime, timezone

class MongoDB:
    def __init__(self, addr: str = 'localhost', port: int = 27017) -> None:
        self.__client = MongoClient(addr, port)

        self.__gitLabAnalyzerDB = self.__client["GitLabAnalyzer"]
        self.__userColl = self.__gitLabAnalyzerDB["users"]
        self.__projectColl = self.__gitLabAnalyzerDB["projects"]
        self.__mergeRequestColl = self.__gitLabAnalyzerDB["mergeRequests"]
        self.__commitColl = self.__gitLabAnalyzerDB["commits"]
        self.__codeDiffColl = self.__gitLabAnalyzerDB["codeDiffs"]
        self.__commentColl = self.__gitLabAnalyzerDB["comments"]
        self.__memberColl = self.__gitLabAnalyzerDB["members"]
        self.__issueColl = self.__gitLabAnalyzerDB["issues"]

    # ********************** INSERT METHODS ******************************

    # NOTE: what about duplicate insertions?
    @staticmethod
    def __insertOne(coll: Collection, body: dict) -> bool:
        try:
            result: InsertOneResult = coll.insert_one(body)
            return result.acknowledged
        except DuplicateKeyError:
            print("MongoDB_interface: Duplicate insert in collection:{}. body:{}".format(coll, body))
            return False

    def insert_GLAUser(self, hashedToken: str, config: dict) -> bool:
        # STUB
        body: dict = {
            '_id': hashedToken,
            'hashed_token': hashedToken,
            'config': config
        }
        return self.__insertOne(self.__userColl, body)

    def insert_project(self, glProject: GitLabProject, projectConfig: dict) -> bool:
        memberIDs: List[int] = []
        for member in glProject.member_manager.get_member_list():
            memberIDs.append(member.id)

        body: dict = {
            '_id': glProject.project_id,
            'project_id': glProject.project_id,
            'name': glProject.project.name,
            'path': glProject.project.path,
            'namespace': {
                'name': glProject.project.namespace,
                'path': glProject.project.path_namespace
            },
            'last_cached_date': datetime.utcnow().replace(tzinfo=timezone.utc, microsecond=0).isoformat(),
            'config': projectConfig,
            'member_ids': memberIDs,
            'user_list': glProject.user_list
        }
        return self.__insertOne(self.__projectColl, body)

    def insert_many_MRs(self, projectID, mergeRequestList: List[MergeRequest]) -> bool:
        body: List[dict] = []

        for mr in mergeRequestList:
            contributors: set = set()
            relatedCommitIDs: list = []
            for commit in mr.related_commits_list:
                contributors.add(commit.author_name)
                relatedCommitIDs.append(commit.id)
            
            body.append({
                'mr_id': mr.id,
                'project_id': projectID,
                'issue_id': mr.__related_issue_iid,
                'merged_date': mr.merged_date,
                'contributors': list(contributors),
                'related_commit_ids': list(relatedCommitIDs)
            })
        result: InsertManyResult = self.__mergeRequestColl.insert_many(body)
        return result.acknowledged
    
    def insert_one_MR(self, projectID, mergeRequest: MergeRequest) -> bool:
        contributors: set = set()
        relatedCommitIDs: list = []
        for commit in mergeRequest.related_commits_list:
            contributors.add(commit.author_name)
            relatedCommitIDs.append(commit.id)

        body: dict = {
            "_id": (mergeRequest.id, projectID),
            'mr_id': mergeRequest.id,
            'project_id': projectID,
            'issue_id': mergeRequest.__related_issue_iid,
            'merged_date': mergeRequest.merged_date,
            'contributors': list(contributors),
            'related_commit_ids': list(relatedCommitIDs)
        }
        return self.__insertOne(self.__mergeRequestColl, body)

    # precondition: all commits in the list belongs to the same mergeRequest.
    #   if they are all master commits, put None for mergeRequestID.
    def insert_many_commits(self, projectID, mergeRequestID, commitList: List[Commit]) -> bool:
        body: List[dict] = []
        for commit in commitList:
            body.append({
                'commit_id': commit.id,
                'project_id': projectID,
                'mr_id': mergeRequestID,
                'author': commit.author_name,
                'commit_date': commit.committed_date,
                'code_diff_id': commit.code_diff_id
            })
        result: InsertManyResult = self.__commitColl.insert_many(body)
        return result.acknowledged

    # if the commit is a commit on the master branch, put None for mergeRequestID
    def insert_one_commit(self, projectID, mergeRequestID, commit: Commit) -> bool:
        body: dict = {
            "_id": (commit.id, projectID),
            'commit_id': commit.id,
            'project_id': projectID,
            'mr_id': mergeRequestID,
            'author': commit.author_name,
            'commiter': commit.committer_name,
            'commit_date': commit.committed_date,
            'code_diff_id': commit.code_diff_id
        }
        return self.__insertOne(self.__codeDiffColl, body)

    # precondition: the list of codeDiffs are in the order where their index is their artif_id
    def insert_many_codeDiffs(self, projectID, codeDiffList: List[List[dict]]) -> bool:
        body: List[dict] = []
        for index, codeDiff in zip(range(len(codeDiffList)), codeDiffList):
            body.append({
                "project_id": projectID,
                "artif_id": index,
                "diffs": codeDiff
            })
        result: InsertManyResult = self.__codeDiffColl.insert_many(body)
        return result.acknowledged

    def insert_one_codeDiff(self, projectID, codeDiffID: int, diffs: List[dict]) -> bool:
        body: dict = {
            "_id": (codeDiffID, projectID),
            "project_id": projectID,
            "artif_id": codeDiffID,
            "diffs": diffs
        }
        return self.__insertOne(self.__codeDiffColl, body)

    def insert_many_comments(self, projectID, commentList: List[Comment]) -> bool:
        body: List[dict] = []
        for comment in commentList:
            body.append({"project": projectID}.update(comment.to_dict))
        result: InsertManyResult = self.__commentColl.insert_many(body)
        return result.acknowledged

    def insert_one_comment(self, projectID, comment: Comment) -> bool:
        body: dict = {"project_id": projectID}
        body.update(comment.to_dict())
        result: InsertOneResult = self.__commentColl.insert_one(body)
        return result.acknowledged

    def insert_many_members(self, memberList: List[Member]) -> bool:
        body: List[dict] = []
        for member in memberList:
            memberObj = {"_id": member.id}
            memberObj.update(member.to_dict())
            body.append(memberObj)
        try:
            result: InsertManyResult = self.__memberColl.insert_many(body)
            return result.acknowledged
        except DuplicateKeyError:
            print("MongoDB_interface::insert_many_members(): Duplicate insert(s)")
            return False

    def insert_one_member(self, member: Member) -> bool:
        memberDict: dict = member.to_dict()
        body: dict = {"_id": member.id}
        body.update(memberDict)
        return self.__insertOne(self.__memberColl, body)

    # TODO: insert Issue methods
    def insert_one_issue(self, issue: Issue) -> bool:
        body: dict = {
            "_id": (issue.issue_id, issue.project_id),
            "project_id": issue.project_id,
            "issue_id": issue.issue_id,
            "author_id": issue.author_id,
            "merge_requests_count": issue.merge_requests_count,
            "commment_count": issue.comment_count,
            "title": issue.title,
            "description": issue.description,
            "state": issue.state,
            "updated_date": issue.updated_date,
            "created_date": issue.created_date,
            "closed_date": issue.closed_date,
            "assignee_id_list": issue.assignee_id_list
        }
        return self.__insertOne(self.__issueColl, body)

    @property
    def collections(self) -> List[str]:
        return self.__gitLabAnalyzerDB.list_collection_names()
        

if __name__ == '__main__':
    # root:pass@mangodb
    testDB = MongoDB('localhost', 27017)

    userObj = {"name": "John", "repoInfo": "this is a test"}

    print(testDB.collections)

"""
DRAFT SCHEMA

// PRIMARY KEY: (_id)
Users Collection: [
	{
		hashed_token: (the user's hashed token)
		config: <user's global config>

        NOTE: config will probably contain info relating to modifications
                relating to what the user can see on screen PER PROJECT.
                Ex. Ignored commits, score multipliers, etc.
	}
]

// PRIMARY KEY: (_id)
Project Collection: [
    {
        project_id: <project id>,
        name: <project name>,
        path: <project path>,
        namespace: {
            name: <name namespace>
            path: <path namespace>
        },
        last_cached_date: <date project was last updated in mongodb>,
        config: {
            <project config. Starts empty, and gets user's config when project is analyzed for the first time.
             this config will include the mapping of user names and members>
        },
        member_ids: [
            <will contain all contributors of the project (members and users)>
        ],
        user_list: [
            <users list>
        ]
    }
]

// PRIMARY KEY: (mr_id, project_id)
MergeRequest Collection: [
    {
        mr_id: <merge request id (project scoped id)>,
        project_id: <id of project this MR belongs to>,
        merged_date: <date when MR was merged>,
        contributors: [
            <people who worked on this MR>
        ],
        related_commit_ids: [
            <ids of commits in the merge request>
        ]
    }
]

// PRIMARY KEY: (commit_id, project_id)
Commit Collection: [
    {
        commit_id: <id of commit>,
        project_id: <id of project this commit belongs to>,
        mr_id: <id of MR this commit is related to OR NULL if master commit>,
        commiter: <person who made this commit>,
        commit_date: <date of commit ISO 8601>
        code_diff_ids: [
            <ids of code diffs for this commit>
        ]
    }
]

// PRIMARY KEY: (artif_id, project_id)
CodeDiff Collection: [
    {
        project_id: <project id>,
        artif_id: 0,
        "a_mode": "0",
        "b_mode": "100644",
        "deleted_file": false,
        "diff": "@@ -0,0 +1,100 @@\n+import copy\n+\n+class Analyzer:\n+    def __init__(self, acceptor_ids, factor=0.05):\n+        self.weight_changed = False\n+        self.acceptor_ids = acceptor_ids\n+        self.factor=factor\n+        self.num_acceptors = len(acceptor_ids)\n+        self.nominal = 1 / self.num_acceptors\n+        #self.ceiling = (int(self.num_acceptors/3)+1) * self.nominal\n+        self.ceiling = 1/2\n+\n+        self.weights = {}\n+        self.msgs_sent = {}\n+        self.msgs_recvd = {}\n+        self.msg_ratios = {}\n+        self.thresholds = {}\n+        for pid in acceptor_ids:\n+            self.weights[pid] = self.nominal\n+            self.msgs_sent[pid] = 0\n+            self.msgs_recvd[pid] = 0\n+            self.msg_ratios[pid] = 0\n+            self.thresholds[pid] = round(1-self.factor,2)\n+\n+        # make copy of state to compare before/after run\n+        self._pre = copy.copy(self)\n+\n+    def add_send(self, pid):\n+        self.msgs_sent[pid] += 1\n+\n+    def add_recvd(self, pid):\n+        self.msgs_recvd[pid] += 1\n+        try:\n+            self.msg_ratios[pid] = round(self.msgs_recvd[pid]/self.msgs_sent[pid],2)\n+        except ZeroDivisionError:\n+            pass\n+        #self.check_threshold(pid)\n+\n+    def check(self):\n+        for pid in self.acceptor_ids:\n+            self.check_threshold(pid)\n+\n+    def check_threshold(self, pid):\n+        if self.msg_ratios[pid] <= self.thresholds[pid]:\n+            self.thresholds[pid] = round(self.thresholds[pid]-self.factor,2)\n+            self.lower_weight(pid)\n+\n+    def lower_weight(self, pid):\n+        # lower the pid's weight\n+        tmp = round(self.weights[pid]-self.factor,2)\n+        if tmp > 0.0:\n+            self.weights[pid] = tmp\n+        else:\n+            self.weights[pid] = 0.0\n+        self.raise_weight()\n+        self.weight_changed = True\n+\n+    def raise_weight(self):\n+        # increase another pid's weight\n+        if self.nominal != self.ceiling:\n+            adjusted = False\n+            while not adjusted:\n+                for i in range(self.num_acceptors):\n+                    pid = self.acceptor_ids[i]\n+                    diff = self.weights[pid] - self.nominal\n+                    if diff == 0.0:\n+                        self.weights[pid] = round(self.weights[pid]+self.factor,2)\n+                        adjusted = True\n+                        break\n+                if i is (self.num_acceptors-1):\n+                    self.nominal = round(self.nominal+self.factor,2)\n+\n+    def log(self):\n+        print(\"Acceptor weights: {}\".format(self.weights))\n+        print(\"Acceptor ratios: {}\".format(self.msg_ratios))\n+        print(\"Acceptor thresholds: {}\".format(self.thresholds))\n+\n+\n+if __name__ == '__main__':\n+    import random\n+\n+    def test(acceptors, fail_rates, num_msgs):\n+        print(\"testing pids {}: \".format(acceptors))\n+        a=Analyzer(acceptors)\n+        print(\"\\nBefore rounds...\")\n+        a.log()\n+        for n in range(num_msgs):\n+            for pid in acceptors:\n+                a.add_send(pid)\n+                if fail_rates[pid] > random.random():\n+                    pass\n+                else:\n+                    a.add_recvd(pid)\n+        print(\"\\nAfter rounds...\")\n+        a.log()\n+        print('\\n')\n+\n+    test([0,1,2,3],[0,0,0.05,0],100)\n+    test([0,1,2,3,4],[0,0,0.05,0,0],100)\n+    test([0,1,2,3,4,5,6,7,8,9],[0,0,0.05,0,0,0,0,0,0.1,0],1000)\n\\ No newline at end of file\n",
        "new_file": true,
        "new_path": "server/model/analyzer.py",
        "old_path": "server/model/analyzer.py",
        "renamed_file": false
        "lines_added": 0,
        "lines_deleted": 0,
        "comments_added": 0,
        "comments_deleted": 0,
        "blanks_added": 0,
        "blanks_deleted": 0,
        "spacing_changes": 0,
        "syntax_changes": 0
    }
]

Comment Collection: [
    {
        _id: 
        comment.to_dict()
    }
]

Member Collection: [
    {
        
    }
]

// PRIMARY KEY(project_id, issue_id)
Issue Collection: [
    {
        project_id,
        issue_id,
        author_id,
        merge_requests_count,
        comment_count,
        title: str = gitlab_issue.title
        description: str = gitlab_issue.description
        state: str = gitlab_issue.state
        updated_date: str = gitlab_issue.updated_at
        created_date: str = gitlab_issue.created_at
        closed_date: Optional[str] = gitlab_issue.closed_at
        assignee_id_list: List[int] = [
            member['id'] for member in gitlab_issue.assignees
        ]

    }
]
"""