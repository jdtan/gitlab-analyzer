from typing import Union, Optional, List, Tuple

class Issue:
    """
        attributes:     
            issue_id -> int                 # the issue's number in the context of the project
            upvotes -> int
            downvotes -> int
            merge_requests_count -> int     # number of merge requests made for the issue
            id -> int                       # TODO: find out what this represents

            title -> str
            description -> str
            state -> str                    # where the issue is on the issues board e.g. "Open", "Closed", "In Progress", etc.
            updated_date -> str             # string datetime in ISO 8601 format (updated_at)
            created_date -> str             # string datetime in ISO 8601 format (created_at)
            closed_at -> str                # string datetime in ISO 8601 format (closed_at)
            due_date -> str                 # string datetime in ISO 8601 format
            project_id -> Union[int, str]   # project ID or URL-encoded path of the project

            milestone -> Milestone
            author -> Member

            assignees_list -> List[Member]  # An immutable list of Members assigned to the issue
            comments_list -> List[Comment]  # An immutable list of Comments belonging to the issue
            labels_list -> List[str]        # An immutable list of tags/labels e.g. "Critical", "Back-End", etc.

        methods:
            Getters for all attributes
    """

    def __init__(self) -> None:
        # by default, the attributes will have test values
        self.__issue_id: int = -1
        self.__upvotes: int = -2
        self.__downvotes: int = -3
        self.__merge_requests_count: int = -4

        self.__title: str = 'title here'
        self.__description: str = 'description here'
        self.__state: str = 'state here'
        self.__updated_date: str = 'YYYY-MM-DDTHH:MI:SS.SSSZ'
        self.__created_date: str = 'YYYY-MM-DDTHH:MI:SS.SSSZ'
        self.__closed_date: Optional[str] = 'YYYY-MM-DDTHH:MI:SS.SSSZ'
        self.__due_date: Optional[str] = None
        self.__project_id: Union[int, str] = 'project_id is Union[int, str]'

        self.__milestone: None = None                                           # TODO: low priority, maybe not needed
        self.__author: None = None                                              # TODO

        self.__assignees_list: List[None] = None                                # TODO
        self.__comments_list: List[None] = None                                 # TODO
        self.__labels_list: List[str] = ['Back-End', 'Test']

    # Getters
    @property
    def labels_list(self) -> Tuple[str]:
        return tuple(self._labels_list)

    @property
    def issue_id(self) -> int:
        return self.__issue_id

    @property
    def upvotes(self) -> int:
        return self._upvotes

    @property
    def downvotes(self) -> int:
        return self._downvotes

    @property
    def merge_requests_count(self) -> int:
        return self._merge_requests_count

    @property
    def title(self) -> str:
        return self._title

    @property
    def description(self) -> str:
        return self._description

    @property
    def state(self) -> str:
        return self._state

    @property
    def updated_date(self) -> str:
        return self._updated_date

    @property
    def created_date(self) -> str:
        return self._created_date

    @property
    def closed_date(self) -> Optional[str]:
        return self._closed_date

    @property
    def due_date(self) -> Optional[str]:
        return self._due_date

    @property
    def project_id(self) -> Union[int, str]:
        return self._project_id
