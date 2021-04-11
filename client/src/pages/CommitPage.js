import React from 'react';
import CommitBar from '../components/commits/CommitBar';
import './CommitsPage.css';
import Header from '../components/Header';
import FooterBar from '../components/FooterBar';

const CommitPage = () => {
  return (
    <>
      <Header />
      <div>
        <CommitBar />
      </div>
      <FooterBar />
    </>
  );
};

export default CommitPage;
