import React, { useEffect } from 'react';
import '../App.css';
import '../Shared.css';
import Logo from '../components/Logo';
import LoginBar from '../components/LoginBar';
import { useAuth } from '../context/AuthContext';

function LoginPage() {
  const { user } = useAuth();

  // For testing login functionality
  const loggedState = () => {
    if (user) {
      return <h1>Logged In</h1>;
    }
    return <h1>Logged Out</h1>;
  };

  return (
    <div className="App">
      <div className="main_container">
        <div className="center">
          <div className="m-bot">
            <Logo />
          </div>
          <LoginBar />
          {loggedState()}
        </div>
      </div>
    </div>
  );
}

export default LoginPage;
