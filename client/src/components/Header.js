import React from 'react';
import { Navbar, Nav, Link} from 'react-bootstrap';
import { LinkContainer } from 'react-router-bootstrap';
import FloatBar from './floatbar/FloatBar';
import { Select, Button} from 'antd';
import Logo from './Logo';

const Header = () => {
  return (
    <div>
      <Logo />
      <FloatBar />
      <Navbar bg="light" expand="lg">
        <Navbar.Toggle aria-controls="basic-navbar-nav" />
        <Navbar.Collapse id="basic-navbar-nav">
          <Nav className="mr-auto">
            <LinkContainer to="/overview" test="hello">
              <Nav.Link>Overview</Nav.Link>
            </LinkContainer>
            <LinkContainer to="/commits">
              <Nav.Link>Commits & MRs</Nav.Link>
            </LinkContainer>
            <LinkContainer to="/table">
              <Nav.Link>Issues & Reviews</Nav.Link>
            </LinkContainer>
            <LinkContainer to="/batch">
              <Nav.Link>Batch Processing</Nav.Link>
            </LinkContainer>
            <LinkContainer to="/config">
              <Nav.Link>Config</Nav.Link>
            </LinkContainer>
          </Nav>
          <Select defaultValue="@someone" style={{ width: 150 }} >
              </Select>
        </Navbar.Collapse>
      </Navbar>
    </div>
  );
};

export default Header;
